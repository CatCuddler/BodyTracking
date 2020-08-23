#include "pch.h"

#include <Kore/IO/FileReader.h>
#include <Kore/Graphics4/PipelineState.h>
#include <Kore/Graphics1/Color.h>
#include <Kore/Input/Keyboard.h>
#include <Kore/Input/Mouse.h>
#include <Kore/Audio2/Audio.h>
#include <Kore/Audio1/Audio.h>
#include <Kore/Audio1/Sound.h>
#include <Kore/Audio1/SoundStream.h>
#include <Kore/System.h>
#include <Kore/Log.h>

#include "Settings.h"
#include "EndEffector.h"
#include "Avatar.h"
#include "LivingRoom.h"
#include "Logger.h"

#include "MachineLearningMotionRecognition.h"

#include <algorithm> // std::sort

#ifdef KORE_STEAMVR
#include <Kore/Vr/VrInterface.h>
#include <Kore/Vr/SensorState.h>
#include <Kore/Input/Gamepad.h>
#endif

using namespace Kore;
using namespace Kore::Graphics4;

namespace {
	const int width = 1024;
	const int height = 768;
	
	const bool renderRoom = true;
	const bool renderTrackerAndController = true;
	const bool renderAxisForEndEffector = false;
	
	EndEffector** endEffector;
	const int numOfEndEffectors = 8;// 12;

	// if set to true, only five trackers (hip, left and right forearm, left and right leg) will be required
	const bool onlyUseFullInverseKinematicTrackerSetup = true;
	
	Logger* logger;

	MachineLearningMotionRecognition* motionRecognizer;
	
	double startTime;
	double lastTime;

	
	// Audio cues
	Sound* startRecordingSound;
	Sound* stopRecordingSound;
	
	// Avatar shader
	VertexStructure structure;
	Shader* vertexShader;
	Shader* fragmentShader;
	PipelineState* pipeline;
	
	TextureUnit tex;
	ConstantLocation pLocation;
	ConstantLocation vLocation;
	ConstantLocation mLocation;
	
	// Living room shader
	VertexStructure structure_living_room;
	Shader* vertexShader_living_room;
	Shader* fragmentShader_living_room;
	PipelineState* pipeline_living_room;
	
	TextureUnit tex_living_room;
	ConstantLocation pLocation_living_room;
	ConstantLocation vLocation_living_room;
	ConstantLocation mLocation_living_room;
	ConstantLocation mLocation_living_room_inverse;
	ConstantLocation diffuse_living_room;
	ConstantLocation specular_living_room;
	ConstantLocation specular_power_living_room;
	ConstantLocation lightPosLocation_living_room;
	ConstantLocation lightCount_living_room;
	
	// Keyboard controls
	bool rotate = false;
	bool W, A, S, D = false;
	
	vec4 camUp(0.0f, 1.0f, 0.0f, 0.0f);
	vec4 camForward(0.0f, 0.0f, 1.0f, 0.0f);
	vec4 camRight(1.0f, 0.0f, 0.0f, 0.0f);
	
	vec3 cameraPos(0, 0, 0);
	
	// Null terminated array of MeshObject pointers (Vive Controller and Tracker)
	MeshObject* viveObjects[] = { nullptr, nullptr, nullptr };
	Avatar* avatar;
	LivingRoom* livingRoom;
	
	// Variables to mirror the room and the avatar
	vec3 mirrorOver(6.057f, 0.0f, 0.04f);
	
	mat4 initTrans;
	mat4 initTransInv;
	Kore::Quaternion initRot;
	Kore::Quaternion initRotInv;
	
	// safety checks for machine learning motion recognition
	bool calibratedAvatarScale = false;
	bool calibratedAvatarControllersAndTrackers = false;
	bool blockAllGamepadButtons = false;
	
#ifdef KORE_STEAMVR
	bool controllerButtonsInitialized = false;
	float currentUserHeight;
	bool firstPersonMonitor = false;
#else
	int loop = 0;
#endif
	
	void renderVRDevice(int index, Kore::mat4 M) {
		Graphics4::setMatrix(mLocation, M);
		viveObjects[index]->render(tex);
	}
	
	mat4 getMirrorMatrix() {
		Kore::Quaternion rot(0, 0, 0, 1);
		rot.rotate(Kore::Quaternion(vec3(0, 1, 0), Kore::pi));
		mat4 zMirror = mat4::Identity();
		zMirror.Set(2, 2 , -1);
		Kore::mat4 M = zMirror * mat4::Translation(mirrorOver.x(), mirrorOver.y(), mirrorOver.z()) * rot.matrix().Transpose();
		
		return M;
	}

	// Whether the user has completet all calibration steps, 
	// namely size calibration and sensor position calibration 
	bool calibratedAvatarCompletely() {
		return (calibratedAvatarScale && calibratedAvatarControllersAndTrackers);
	}
	
	void renderControllerAndTracker(int tracker, Kore::vec3 desPosition, Kore::Quaternion desRotation) {
		// World Transformation Matrix
		Kore::mat4 W = mat4::Translation(desPosition.x(), desPosition.y(), desPosition.z()) * desRotation.matrix().Transpose();
		
		// Mirror Transformation Matrix
		Kore::mat4 M = getMirrorMatrix() * W;
		
		if (tracker) {
			// Render a tracker for both feet and back
			renderVRDevice(0, W);
			renderVRDevice(0, M);
		} else {
			// Render a controller for both hands
			renderVRDevice(1, W);
			renderVRDevice(1, M);
		}
		
		// Render a local coordinate system only if the avatar is not calibrated
		if (!calibratedAvatarCompletely()) {
			renderVRDevice(2, W);
			renderVRDevice(2, M);
		}
	}
	
	void renderAllVRDevices() {
		Graphics4::setPipeline(pipeline);
	
#ifdef KORE_STEAMVR
		VrPoseState controller;
		for (int i = 0; i < 16; ++i) {
			controller = VrInterface::getController(i);
			
			vec3 desPosition = controller.vrPose.position;
			Kore::Quaternion desRotation = controller.vrPose.orientation;
			
			if (controller.trackedDevice == TrackedDevice::ViveTracker) {
				renderControllerAndTracker(true, desPosition, desRotation);
			} else if (controller.trackedDevice == TrackedDevice::Controller) {
				renderControllerAndTracker(false, desPosition, desRotation);
			}
			
		}
#else
		for(int i = 0; i < numOfEndEffectors; ++i) {
			Kore::vec3 desPosition = endEffector[i]->getDesPosition();
			Kore::Quaternion desRotation = endEffector[i]->getDesRotation();
			
			if (i == hip || i == leftForeArm || i == rightForeArm || i == leftFoot || i == rightFoot) {
				renderControllerAndTracker(true, desPosition, desRotation);
			} else if (i == rightHand || i == leftHand) {
				renderControllerAndTracker(false, desPosition, desRotation);
			}
		}
#endif
	}
	
	void renderCSForEndEffector() {
		Graphics4::setPipeline(pipeline);
		
		for(int i = 0; i < numOfEndEffectors; ++i) {
			BoneNode* bone = avatar->getBoneWithIndex(endEffector[i]->getBoneIndex());
			
			vec3 endEffectorPos = bone->getPosition();
			endEffectorPos = initTrans * vec4(endEffectorPos.x(), endEffectorPos.y(), endEffectorPos.z(), 1);
			Kore::Quaternion endEffectorRot = initRot.rotated(bone->getOrientation());
			
			Kore::mat4 M = mat4::Translation(endEffectorPos.x(), endEffectorPos.y(), endEffectorPos.z()) * endEffectorRot.matrix().Transpose();
			Graphics4::setMatrix(mLocation, M);
			viveObjects[2]->render(tex);
		}
	}
	
	void renderLivingRoom(mat4 V, mat4 P) {
		Graphics4::setPipeline(pipeline_living_room);
		
		livingRoom->setLights(lightCount_living_room, lightPosLocation_living_room);
		Graphics4::setMatrix(vLocation_living_room, V);
		Graphics4::setMatrix(pLocation_living_room, P);
		livingRoom->render(tex_living_room, mLocation_living_room, mLocation_living_room_inverse, diffuse_living_room, specular_living_room, specular_power_living_room, false);
		
		livingRoom->render(tex_living_room, mLocation_living_room, mLocation_living_room_inverse, diffuse_living_room, specular_living_room, specular_power_living_room, true);
	}
	
	void renderAvatar(mat4 V, mat4 P) {
		Graphics4::setPipeline(pipeline);
		
		Graphics4::setMatrix(vLocation, V);
		Graphics4::setMatrix(pLocation, P);
		Graphics4::setMatrix(mLocation, initTrans);
		avatar->animate(tex);
		
		// Mirror the avatar
		mat4 initTransMirror = getMirrorMatrix() * initTrans;
		
		Graphics4::setMatrix(mLocation, initTransMirror);
		avatar->animate(tex);
	}
	
	Kore::mat4 getProjectionMatrix() {
		mat4 P = mat4::Perspective(45, (float)width / (float)height, 0.01f, 1000);
		P.Set(0, 0, -P.get(0, 0));
		
		return P;
	}
	
	Kore::mat4 getViewMatrix() {
		mat4 V = mat4::lookAlong(camForward.xyz(), cameraPos, vec3(0.0f, 1.0f, 0.0f));
		return V;
	}
	

	// Convert a 3D vector to a quaternion
	Kore::Quaternion toQuaternion(vec3 vector3D)
	{
		double cosinusYaw = cos(vector3D.z() * 0.5);
		double sinusYaw = sin(vector3D.z() * 0.5);
		double cosinusPitch = cos(vector3D.y() * 0.5);
		double sinusPitch = sin(vector3D.y() * 0.5);
		double cosinusRoll = cos(vector3D.x() * 0.5);
		double sinusRoll = sin(vector3D.x() * 0.5);

		Kore::Quaternion quaternion;

		quaternion.w = cosinusYaw * cosinusPitch * cosinusRoll + sinusYaw * sinusPitch * sinusRoll;
		quaternion.x = cosinusYaw * cosinusPitch * sinusRoll - sinusYaw * sinusPitch * cosinusRoll;
		quaternion.y = sinusYaw * cosinusPitch * sinusRoll + cosinusYaw * sinusPitch * cosinusRoll;
		quaternion.z = sinusYaw * cosinusPitch * cosinusRoll - cosinusYaw * sinusPitch * sinusRoll;

		return quaternion;
	}

	// Convert a quaternion to euler angles
	vec3 toEulerAngles(const Kore::Quaternion q)
	{
		// roll (x-axis rotation)
		double roll;
		double sinr_cosp = +2.0 * (q.w * q.x + q.y * q.z);
		double cosr_cosp = +1.0 - 2.0 * (q.x * q.x + q.y * q.y);
		roll = atan2(sinr_cosp, cosr_cosp);

		// pitch (y-axis rotation)
		double pitch;
		double sinp = +2.0 * (q.w * q.y - q.z * q.x);
		if (fabs(sinp) >= 1)
			pitch = copysign(Kore::pi / 2, sinp); // use 90 degrees if out of range
		else
			pitch = asin(sinp);

		// yaw (z-axis rotation)
		double yaw;
		double siny_cosp = +2.0 * (q.w * q.z + q.x * q.y);
		double cosy_cosp = +1.0 - 2.0 * (q.y * q.y + q.z * q.z);
		yaw = atan2(siny_cosp, cosy_cosp);

		return vec3(roll, pitch, yaw);
	}


	void executeMovement(int endEffectorID) {

		Kore::vec3 desPosition = endEffector[endEffectorID]->getDesPosition();
		Kore::Quaternion desRotation = endEffector[endEffectorID]->getDesRotation();

		// Save raw data
		if (logRawData) logger->saveData(endEffector[endEffectorID]->getName(), desPosition, desRotation, avatar->scale);
		
		//if (calibratedAvatar) {
		
		if (calibratedAvatarControllersAndTrackers) {
			// Transform desired position/rotation to the character local coordinate system
			desPosition = initTransInv * vec4(desPosition.x(), desPosition.y(), desPosition.z(), 1);
			desRotation = initRotInv.rotated(desRotation);

			// Add offset
			Kore::Quaternion offsetRotation = endEffector[endEffectorID]->getOffsetRotation();
			vec3 offsetPosition = endEffector[endEffectorID]->getOffsetPosition();
			Kore::Quaternion finalRot = desRotation.rotated(offsetRotation);
			vec3 finalPos = mat4::Translation(desPosition.x(), desPosition.y(), desPosition.z()) * finalRot.matrix().Transpose() * mat4::Translation(offsetPosition.x(), offsetPosition.y(), offsetPosition.z()) * vec4(0, 0, 0, 1);
			
			if (endEffectorID == hip) {
				avatar->setFixedPositionAndOrientation(endEffector[endEffectorID]->getBoneIndex(), finalPos, finalRot);
			} else if (endEffectorID == head || endEffectorID == leftForeArm || endEffectorID == rightForeArm || endEffectorID == leftFoot || endEffectorID == rightFoot) {
				avatar->setDesiredPositionAndOrientation(endEffector[endEffectorID]->getBoneIndex(), endEffector[endEffectorID]->getIKMode(), finalPos, finalRot);
			} else if (endEffectorID == leftHand || endEffectorID == rightHand) {
				avatar->setFixedOrientation(endEffector[endEffectorID]->getBoneIndex(), finalRot);
			}
			
			// Save calibrated data to either train Motion Recognizer or to recognize a movement
			if (motionRecognizer->isProcessingMovementData()) {

				// if we are not using actual VR sensors, we cannot retrieve the velocity values and have to use defaults
				// if we do use VR sensors, the actual velocity can be used
				vec3 rawAngVel;
				Kore::Quaternion desAngVel;
				vec3 rawLinVel;
				vec3 desLinVel;
				vec3 rawPosition;
				Kore::Quaternion rawRotation;

				// actual VR hardware present
				#ifdef KORE_STEAMVR

					VrPoseState sensorState;
					// retrieve sensor or HMD state (data is the same, retrieval is slightly different)
					if (endEffectorID == head) {
						sensorState = VrInterface::getSensorState(0).pose;
					} else {
						sensorState = VrInterface::getController(endEffector[endEffectorID]->getDeviceIndex());
					}

					// collect data
					rawLinVel = sensorState.linearVelocity;
					rawAngVel = sensorState.angularVelocity;
					desLinVel = initTransInv * vec4(rawLinVel.x(), rawLinVel.y(), rawLinVel.z(), 1);
					Kore::Quaternion rawAngVelQuat = toQuaternion(rawAngVel);
					desAngVel = initRotInv.rotated(rawAngVelQuat);
					rawPosition = sensorState.vrPose.position;
					rawRotation = sensorState.vrPose.orientation;

				// no actual VR hardware present
				#else
					// these placeholder values are only used for testing with predetermined movement sets, not for recording new data
					rawAngVel = vec3(1, 2, 3);
					desAngVel = desRotation;
					rawLinVel = vec3(7, 8, 9);
					desLinVel = rawLinVel;
					rawPosition = desPosition;
					rawRotation = desRotation;
				#endif

				// forward data
				motionRecognizer->processMovementData(
					endEffector[endEffectorID]->getName(), 
					rawPosition, desPosition, finalPos, 
					rawRotation, desRotation, finalRot, 
					rawAngVel, desAngVel, 
					rawLinVel, desLinVel, 
					avatar->scale, lastTime);
			}
		}
	}

	
	void initTransAndRot() {
		initRot = Kore::Quaternion(0, 0, 0, 1);
		initRot.rotate(Kore::Quaternion(vec3(1, 0, 0), -Kore::pi / 2.0));
		initRot.rotate(Kore::Quaternion(vec3(0, 0, 1), Kore::pi / 2.0));
		initRot.normalize();
		initRotInv = initRot.invert();
		
		vec3 initPos = vec4(0, 0, 0, 1);
		initTrans = mat4::Translation(initPos.x(), initPos.y(), initPos.z()) * initRot.matrix().Transpose();
		initTransInv = initTrans.Invert();
	}
	
	void calibrate() {
		initTransAndRot();
		
		for (int i = 0; i < numOfEndEffectors; ++i) {
			Kore::vec3 desPosition = endEffector[i]->getDesPosition();
			Kore::Quaternion desRotation = endEffector[i]->getDesRotation();
			
			// Transform desired position/rotation to the character local coordinate system
			desPosition = initTransInv * vec4(desPosition.x(), desPosition.y(), desPosition.z(), 1);
			desRotation = initRotInv.rotated(desRotation);
			
			// Get actual position/rotation of the character skeleton
			BoneNode* bone = avatar->getBoneWithIndex(endEffector[i]->getBoneIndex());
			vec3 targetPos = bone->getPosition();
			Kore::Quaternion targetRot = bone->getOrientation();
			
			endEffector[i]->setOffsetPosition((mat4::Translation(desPosition.x(), desPosition.y(), desPosition.z()) * targetRot.matrix().Transpose()).Invert() * mat4::Translation(targetPos.x(), targetPos.y(), targetPos.z()) * vec4(0, 0, 0, 1));
			endEffector[i]->setOffsetRotation((desRotation.invert()).rotated(targetRot));
		}
	}
	
	void record() {
		logRawData = !logRawData;
		
		if (!motionRecognizer->isActive()) {
			if (logRawData) {
				Audio1::play(startRecordingSound);
				logger->startLogger("logData");
			} else {
				Audio1::play(stopRecordingSound);
				logger->endLogger();
			}
		}
	}

#ifdef KORE_STEAMVR
	void setSize() {
		float currentAvatarHeight = avatar->getHeight();
		
		SensorState state = VrInterface::getSensorState(0);
		vec3 hmdPos = state.pose.vrPose.position; // z -> face, y -> up down
		currentUserHeight = hmdPos.y();
		
		float scale = currentUserHeight / currentAvatarHeight;
		avatar->setScale(scale);

		calibratedAvatarScale = true;
		
		log(Info, "current avatar height %f, current user height %f ==> scale %f", currentAvatarHeight, currentUserHeight, scale);
	}
	
	void initEndEffector(int efID, int deviceID, vec3 pos, Kore::Quaternion rot) {
		endEffector[efID]->setDeviceIndex(deviceID);
		endEffector[efID]->setDesPosition(pos);
		endEffector[efID]->setDesRotation(rot);
		
		log(Info, "%s, device id: %i", endEffector[efID]->getName(), deviceID);
	}
	
	void assignControllerAndTracker() {
		VrPoseState vrDevice;
		
		int numTrackers;
		if (onlyUseFullInverseKinematicTrackerSetup) {
			numTrackers = 5;
		}
		else {
			numTrackers = 9;
		}

		int trackerCount = 0;
		
		std::vector<EndEffector*> trackers;
		
		// Get indices for VR devices
		for (int i = 0; i < 16; ++i) {
			vrDevice = VrInterface::getController(i);
			
			vec3 devicePos = vrDevice.vrPose.position;
			Kore::Quaternion deviceRot = vrDevice.vrPose.orientation;

			if (vrDevice.trackedDevice == TrackedDevice::ViveTracker) {
				EndEffector* tracker = new EndEffector(-1);
				tracker->setDeviceIndex(i);
				tracker->setDesPosition(devicePos);
				tracker->setDesRotation(deviceRot);
				trackers.push_back(tracker);
				
				++trackerCount;
				if (trackerCount == numTrackers) {
					// Sort trackers regarding the y-Axis (height)
					std::sort(trackers.begin(), trackers.end(), sortByYAxis());

					// if requested, use only the 2 leg trackers, the 2 forearm trackers, and the hip tracker
					if (onlyUseFullInverseKinematicTrackerSetup) {

						// Left or Right Leg
						// Sort first two trackers regarding the z-Axis (left-right)
						std::sort(trackers.begin(), trackers.begin() + 2, sortByZAxis());
						initEndEffector(leftFoot, trackers[0]->getDeviceIndex(), trackers[0]->getDesPosition(), trackers[0]->getDesRotation());
						initEndEffector(rightFoot, trackers[1]->getDeviceIndex(), trackers[1]->getDesPosition(), trackers[1]->getDesRotation());

						// Hip
						initEndEffector(hip, trackers[2]->getDeviceIndex(), trackers[2]->getDesPosition(), trackers[2]->getDesRotation());
						
						// Left Fore Arm or Right Fore Arm
						std::sort(trackers.begin() + 3, trackers.begin() + 5, sortByZAxis());
						initEndEffector(leftForeArm, trackers[3]->getDeviceIndex(), trackers[3]->getDesPosition(), trackers[3]->getDesRotation());
						initEndEffector(rightForeArm, trackers[4]->getDeviceIndex(), trackers[4]->getDesPosition(), trackers[4]->getDesRotation());

					}
					// otherwise, use all 9 trackers
					else {

						// Left or Right Foot
						// Sort first two trackers regarding the z-Axis (left-right)
						std::sort(trackers.begin(), trackers.begin() + 2, sortByZAxis());
						initEndEffector(leftFoot, trackers[0]->getDeviceIndex(), trackers[0]->getDesPosition(), trackers[0]->getDesRotation());
						initEndEffector(rightFoot, trackers[1]->getDeviceIndex(), trackers[1]->getDesPosition(), trackers[1]->getDesRotation());
					
						// Left or Right Leg
						std::sort(trackers.begin()+2, trackers.begin()+4, sortByZAxis());
						initEndEffector(leftFoot, trackers[2]->getDeviceIndex(), trackers[2]->getDesPosition(), trackers[2]->getDesRotation());
						initEndEffector(rightFoot, trackers[3]->getDeviceIndex(), trackers[3]->getDesPosition(), trackers[3]->getDesRotation());
					
						// Hip
						initEndEffector(hip, trackers[4]->getDeviceIndex(), trackers[4]->getDesPosition(), trackers[4]->getDesRotation());
					
						// Spine
						initEndEffector(spine, trackers[5]->getDeviceIndex(), trackers[5]->getDesPosition(), trackers[5]->getDesRotation());
					
						// Left Fore Arm, Right Fore Arm or Right Arm
						std::sort(trackers.begin()+6, trackers.begin()+9, sortByZAxis());
						initEndEffector(leftForeArm, trackers[6]->getDeviceIndex(), trackers[6]->getDesPosition(), trackers[6]->getDesRotation());
						initEndEffector(rightHand, trackers[7]->getDeviceIndex(), trackers[7]->getDesPosition(), trackers[7]->getDesRotation());
						initEndEffector(rightForeArm, trackers[8]->getDeviceIndex(), trackers[8]->getDesPosition(), trackers[8]->getDesRotation());
					}
				}
				
				
			} else if (vrDevice.trackedDevice == TrackedDevice::Controller) {
				// Hand controller
				if (devicePos.z() > 0) {
					initEndEffector(rightHand, i, devicePos, deviceRot);
				} else {
					initEndEffector(leftHand, i, devicePos, deviceRot);
				}
			}
		}
		
		// HMD
		SensorState stateLeftEye = VrInterface::getSensorState(0);
		SensorState stateRightEye = VrInterface::getSensorState(1);
		vec3 leftEyePos = stateLeftEye.pose.vrPose.position;
		vec3 rightEyePos = stateRightEye.pose.vrPose.position;
		vec3 hmdPosCenter = (leftEyePos + rightEyePos) / 2;
		initEndEffector(head, 0, hmdPosCenter, stateLeftEye.pose.vrPose.orientation);
	}
	
	void gamepadButton(int buttonNr, float value) {

		// disable buttons if Motion Recognizer is currently recording data, to avoid e.g. avatar re-calibration
		if (motionRecognizer->isRecordingMovementData()) {
			Kore::log(Kore::LogLevel::Warning,
				"Gamepad Buttons are disabled during Movement Data recording");
			return;
		}

		// disable buttons if they have been locked by keyboard command
		if (blockAllGamepadButtons) {
			Kore::log(Kore::LogLevel::Warning,
				"Gamepad Buttons have been disabled by keyboard");
			return;		
		}

		// Grip button => set size and reset an avatar to a default T-Pose
		if (buttonNr == 2 && value == 1) {
			calibratedAvatarControllersAndTrackers = false;
			initTransAndRot();
			avatar->resetPositionAndRotation();
			setSize();
		}
		
		// Menu button => calibrate
		if (buttonNr == 1 && value == 1) {
			assignControllerAndTracker();
			calibrate();
			calibratedAvatarControllersAndTrackers = true;
			log(Info, "Calibrate avatar");
		}
		
		// Trigger button => record data
		if (buttonNr == 33 && value == 1) {
			record();
		}
	}
	
	void initButtons() {
		VrPoseState controller;

		int count = 0;
		
		for (int i = 0; i < 16; ++i) {
			controller = VrInterface::getController(i);
			
			if (controller.trackedDevice == TrackedDevice::Controller) {
				Gamepad::get(i)->Button = gamepadButton;
				++count;
				log(Info, "Add gamepad controller %i", count);
			}
		}

		assert(count == 2);
		controllerButtonsInitialized = true;
	}
#endif
	void update() {
		float t = (float)(System::time() - startTime);
		double deltaT = t - lastTime;
		lastTime = t;
		
		// Move position of camera based on WASD keys
		float cameraMoveSpeed = 4.f;
		if (S) cameraPos -= camForward * (float)deltaT * cameraMoveSpeed;
		if (W) cameraPos += camForward * (float)deltaT * cameraMoveSpeed;
		if (A) cameraPos += camRight * (float)deltaT * cameraMoveSpeed;
		if (D) cameraPos -= camRight * (float)deltaT * cameraMoveSpeed;
		
		Graphics4::begin();
		Graphics4::clear(Graphics4::ClearColorFlag | Graphics4::ClearDepthFlag, Graphics1::Color::Black, 1.0f, 0);
		Graphics4::setPipeline(pipeline);
		
#ifdef KORE_STEAMVR
		VrInterface::begin();

		if (!controllerButtonsInitialized) initButtons();
		
		VrPoseState vrDevice;

		for (int i = 0; i < numOfEndEffectors; ++i) {

			if (endEffector[i]->getDeviceIndex() != -1) {

				if (i == head) {
					SensorState state = VrInterface::getSensorState(0);

					// Get HMD position and rotation
					endEffector[i]->setDesPosition(state.pose.vrPose.position);
					endEffector[i]->setDesRotation(state.pose.vrPose.orientation);
				} else {
					vrDevice = VrInterface::getController(endEffector[i]->getDeviceIndex());

					// Get VR device position and rotation
					endEffector[i]->setDesPosition(vrDevice.vrPose.position);
					endEffector[i]->setDesRotation(vrDevice.vrPose.orientation);
					
					if (calibratedAvatarCompletely() && i == hip) {
						// Update Local Coordinate System
						initTransAndRot();
					}
				}

				executeMovement(i);
			}
		}
		
		// Render for both eyes
		SensorState state;
		for (int j = 0; j < 2; ++j) {
			VrInterface::beginRender(j);
			
			Graphics4::clear(Graphics4::ClearColorFlag | Graphics4::ClearDepthFlag, Graphics1::Color::Black, 1.0f, 0);
			
			state = VrInterface::getSensorState(j);
			
			renderAvatar(state.pose.vrPose.eye, state.pose.vrPose.projection);
			
			if (renderTrackerAndController) renderAllVRDevices();
			
			if (renderAxisForEndEffector) renderCSForEndEffector();
			
			if (renderRoom) renderLivingRoom(state.pose.vrPose.eye, state.pose.vrPose.projection);
			
			VrInterface::endRender(j);
		}
		
		VrInterface::warpSwap();
		
		Graphics4::restoreRenderTarget();
		Graphics4::clear(Graphics4::ClearColorFlag | Graphics4::ClearDepthFlag, Graphics1::Color::Black, 1.0f, 0);
		
		// Render on monitor
		mat4 P = getProjectionMatrix();
		mat4 V = getViewMatrix();

		if (!firstPersonMonitor) renderAvatar(V, P);
		else renderAvatar(state.pose.vrPose.eye, state.pose.vrPose.projection);
		
		if (renderTrackerAndController) renderAllVRDevices();
		
		if (renderAxisForEndEffector) renderCSForEndEffector();
		
		if (renderRoom) {
			if (!firstPersonMonitor) renderLivingRoom(V, P);
			else renderLivingRoom(state.pose.vrPose.eye, state.pose.vrPose.projection);
		}
#else
		// Read line
		float scaleFactor;
		Kore::vec3 desPosition[numOfEndEffectors];
		Kore::Quaternion desRotation[numOfEndEffectors];
		if (currentFile < numFiles) {
			bool dataAvailable = logger->readData(numOfEndEffectors, files[currentFile], desPosition, desRotation, scaleFactor);
			
			for (int i = 0; i < numOfEndEffectors; ++i) {
				endEffector[i]->setDesPosition(desPosition[i]);
				endEffector[i]->setDesRotation(desRotation[i]);
				
				if (calibratedAvatarCompletely() && i == hip) {
					// Update Local Coordinate System
					//initTransAndRot();
				}
			}
			
			if (!calibratedAvatarControllersAndTrackers) {
				avatar->resetPositionAndRotation();
				avatar->setScale(scaleFactor);
				calibratedAvatarScale = true;
				calibrate();
				calibratedAvatarControllersAndTrackers = true;
			}
			
			for (int i = 0; i < numOfEndEffectors; ++i) {
				executeMovement(i);
			}
			
			if (!dataAvailable) {
				currentFile++;
				calibratedAvatarScale = false;
				calibratedAvatarControllersAndTrackers = false;
			}
		}
		
		// Get projection and view matrix
		mat4 P = getProjectionMatrix();
		mat4 V = getViewMatrix();
		
		renderAvatar(V, P);
		
		if (renderTrackerAndController) renderAllVRDevices();
		
		if (renderAxisForEndEffector) renderCSForEndEffector();
		
		if (renderRoom) renderLivingRoom(V, P);
#endif

		Graphics4::end();
		Graphics4::swapBuffers();
	}


	
	void keyDown(KeyCode code) {
		switch (code) {
			case Kore::KeyW:
				W = true;
				break;
			case Kore::KeyA:
				A = true;
				break;
			case Kore::KeyS:
				S = true;
				break;
			case Kore::KeyD:
				D = true;
				break;
			case Kore::KeyR:
#ifdef KORE_STEAMVR
				VrInterface::resetHmdPose();
#endif
				break;
			case KeyL:
				//Kore::log(Kore::LogLevel::Info, "cameraPos: (%f, %f, %f)", cameraPos.x(), cameraPos.y(), cameraPos.z());
				//Kore::log(Kore::LogLevel::Info, "camUp: (%f, %f, %f, %f)", camUp.x(), camUp.y(), camUp.z(), camUp.w());
				//Kore::log(Kore::LogLevel::Info, "camRight: (%f, %f, %f, %f)", camRight.x(), camRight.y(), camRight.z(), camRight.w());
				//Kore::log(Kore::LogLevel::Info, "camForward: (%f, %f, %f, %f)", camForward.x(), camForward.y(), camForward.z(), camForward.w());
				
				record();
				break;
			case Kore::KeyEscape:
			case KeyQ:
				System::stop();
				break;
#ifdef KORE_STEAMVR
				// simulate gamepad inputs, so tester can take control 
				// and avoid wrong inputs from test subjects
			case Kore::KeyH:
				gamepadButton(2, 1);	// set avatar size
				break;
			case Kore::KeyK:
				gamepadButton(1, 1);	// calibrate controllers and trackers
				break;
			case Kore::KeyB:			// disable gamepad buttons
				blockAllGamepadButtons = !blockAllGamepadButtons;
				if (blockAllGamepadButtons) {
					Kore::log(Kore::LogLevel::Info,
						"Gamepad Buttons have been disabled by keyboard");
				}
				else {
					Kore::log(Kore::LogLevel::Info,
						"Gamepad Buttons have been enabled by keyboard");
				}
				break;
#endif
			default:
				break;
		}
		
		// the motion recognizer handles its own input in order to start recording data for specific tasks
		// uses the numpad number keys, and the space bar (if the class is in use)
		if (motionRecognizer->isActive()) {
			motionRecognizer->processKeyDown(code, calibratedAvatarCompletely());
		}
	}
	
	void keyUp(KeyCode code) {
		switch (code) {
			case Kore::KeyW:
				W = false;
				break;
			case Kore::KeyA:
				A = false;
				break;
			case Kore::KeyS:
				S = false;
				break;
			case Kore::KeyD:
				D = false;
				break;
			default:
				break;
		}
	}
	
	void mouseMove(int windowId, int x, int y, int movementX, int movementY) {
		Kore::Quaternion q1(vec3(0.0f, 1.0f, 0.0f), 0.01f * movementX);
		Kore::Quaternion q2(camRight, 0.01f * -movementY);
		
		camUp = q2.matrix() * camUp;
		camRight = q1.matrix() * camRight;
		
		q2.rotate(q1);
		mat4 mat = q2.matrix();
		camForward = mat * camForward;
	}
	
	
	void mousePress(int windowId, int button, int x, int y) {
		rotate = true;
	}
	
	void mouseRelease(int windowId, int button, int x, int y) {
		rotate = false;
	}
	
	void loadAvatarShader() {
		FileReader vs("shader.vert");
		FileReader fs("shader.frag");
		vertexShader = new Shader(vs.readAll(), vs.size(), VertexShader);
		fragmentShader = new Shader(fs.readAll(), fs.size(), FragmentShader);
		
		// This defines the structure of your Vertex Buffer
		structure.add("pos", Float3VertexData);
		structure.add("tex", Float2VertexData);
		structure.add("nor", Float3VertexData);
		
		pipeline = new PipelineState();
		pipeline = new PipelineState();
		pipeline->inputLayout[0] = &structure;
		pipeline->inputLayout[1] = nullptr;
		pipeline->vertexShader = vertexShader;
		pipeline->fragmentShader = fragmentShader;
		pipeline->depthMode = ZCompareLess;
		pipeline->depthWrite = true;
		pipeline->blendSource = Graphics4::SourceAlpha;
		pipeline->blendDestination = Graphics4::InverseSourceAlpha;
		pipeline->alphaBlendSource = Graphics4::SourceAlpha;
		pipeline->alphaBlendDestination = Graphics4::InverseSourceAlpha;
		pipeline->compile();
		
		tex = pipeline->getTextureUnit("tex");
		Graphics4::setTextureAddressing(tex, Graphics4::U, Repeat);
		Graphics4::setTextureAddressing(tex, Graphics4::V, Repeat);
		
		pLocation = pipeline->getConstantLocation("P");
		vLocation = pipeline->getConstantLocation("V");
		mLocation = pipeline->getConstantLocation("M");
	}
	
	void loadLivingRoomShader() {
		FileReader vs("shader_basic_shading.vert");
		FileReader fs("shader_basic_shading.frag");
		vertexShader_living_room = new Shader(vs.readAll(), vs.size(), VertexShader);
		fragmentShader_living_room = new Shader(fs.readAll(), fs.size(), FragmentShader);
		
		structure_living_room.add("pos", Float3VertexData);
		structure_living_room.add("tex", Float2VertexData);
		structure_living_room.add("nor", Float3VertexData);
		
		pipeline_living_room = new PipelineState();
		pipeline_living_room->inputLayout[0] = &structure_living_room;
		pipeline_living_room->inputLayout[1] = nullptr;
		pipeline_living_room->vertexShader = vertexShader_living_room;
		pipeline_living_room->fragmentShader = fragmentShader_living_room;
		pipeline_living_room->depthMode = ZCompareLess;
		pipeline_living_room->depthWrite = true;
		pipeline_living_room->blendSource = Graphics4::SourceAlpha;
		pipeline_living_room->blendDestination = Graphics4::InverseSourceAlpha;
		pipeline_living_room->alphaBlendSource = Graphics4::SourceAlpha;
		pipeline_living_room->alphaBlendDestination = Graphics4::InverseSourceAlpha;
		pipeline_living_room->compile();
		
		tex_living_room = pipeline_living_room->getTextureUnit("tex");
		Graphics4::setTextureAddressing(tex_living_room, Graphics4::U, Repeat);
		Graphics4::setTextureAddressing(tex_living_room, Graphics4::V, Repeat);
		
		pLocation_living_room = pipeline_living_room->getConstantLocation("P");
		vLocation_living_room = pipeline_living_room->getConstantLocation("V");
		mLocation_living_room = pipeline_living_room->getConstantLocation("M");
		mLocation_living_room_inverse = pipeline_living_room->getConstantLocation("MInverse");
		diffuse_living_room = pipeline_living_room->getConstantLocation("diffuseCol");
		specular_living_room = pipeline_living_room->getConstantLocation("specularCol");
		specular_power_living_room = pipeline_living_room->getConstantLocation("specularPow");
		lightPosLocation_living_room = pipeline_living_room->getConstantLocation("lightPos");
		lightCount_living_room = pipeline_living_room->getConstantLocation("numLights");
	}
	
	void init() {
		loadAvatarShader();
        avatar = new Avatar("avatar/avatar_male.ogex", "avatar/", structure);
		//avatar = new Avatar("avatar/avatar_female.ogex", "avatar/", structure);
		
		initTransAndRot();
		
		// Set camera initial position and orientation
		cameraPos = vec3(2.6, 1.8, 0.0);
		Kore::Quaternion q1(vec3(0.0f, 1.0f, 0.0f), Kore::pi / 2.0f);
		Kore::Quaternion q2(vec3(1.0f, 0.0f, 0.0f), -Kore::pi / 8.0f);
		camUp = q2.matrix() * camUp;
		camRight = q1.matrix() * camRight;
		q2.rotate(q1);
		mat4 mat = q2.matrix();
		camForward = mat * camForward;
		
		if (renderTrackerAndController) {
			viveObjects[0] = new MeshObject("vivemodels/vivetracker.ogex", "vivemodels/", structure, 1);
			viveObjects[1] = new MeshObject("vivemodels/vivecontroller.ogex", "vivemodels/", structure, 1);
		}
		
		if (renderTrackerAndController || renderAxisForEndEffector) {
			viveObjects[2] = new MeshObject("vivemodels/axis.ogex", "vivemodels/", structure, 1);
		}
		
		if (renderRoom) {
			loadLivingRoomShader();
			livingRoom = new LivingRoom("sherlock_living_room/sherlock_living_room.ogex", "sherlock_living_room/", structure_living_room, 1);
			Kore::Quaternion livingRoomRot = Kore::Quaternion(0, 0, 0, 1);
			livingRoomRot.rotate(Kore::Quaternion(vec3(1, 0, 0), -Kore::pi / 2.0));
			livingRoomRot.rotate(Kore::Quaternion(vec3(0, 0, 1), Kore::pi / 2.0));
			livingRoom->M = mat4::Translation(0, 0, 0) * livingRoomRot.matrix().Transpose();
			
			mat4 mirrorMatrix = mat4::Identity();
			mirrorMatrix.Set(2, 2, -1);
			livingRoomRot.rotate(Kore::Quaternion(vec3(0, 0, 1), Kore::pi));
			livingRoom->Mmirror = mirrorMatrix * mat4::Translation(mirrorOver.x(), mirrorOver.y(), mirrorOver.z()) * livingRoomRot.matrix().Transpose();
		}
		
		logger = new Logger();

		motionRecognizer = new MachineLearningMotionRecognition(*logger);
		
		endEffector = new EndEffector*[numOfEndEffectors];
		endEffector[head] = new EndEffector(headBoneIndex);
		endEffector[hip] = new EndEffector(hipBoneIndex);
		endEffector[leftHand] = new EndEffector(leftHandBoneIndex);
		endEffector[leftForeArm] = new EndEffector(leftForeArmBoneIndex);
		endEffector[rightHand] = new EndEffector(rightHandBoneIndex);
		endEffector[rightForeArm] = new EndEffector(rightForeArmBoneIndex);
		endEffector[leftFoot] = new EndEffector(leftFootBoneIndex);
		endEffector[rightFoot] = new EndEffector(rightFootBoneIndex);

		// Additional End Effectors (usually not used for IK)
		endEffector[spine] = new EndEffector(spineBoneIndex);
		endEffector[rightArm] = new EndEffector(rightArmBoneIndex);
		endEffector[leftLeg] = new EndEffector(leftLegBoneIndex);
		endEffector[rightLeg] = new EndEffector(rightLegBoneIndex);
		
#ifdef KORE_STEAMVR
		VrInterface::init(nullptr, nullptr, nullptr); // TODO: Remove
#endif
	}
}

int kickstart(int argc, char** argv) {
	System::init("BodyTracking", width, height);
	
	init();
	
	System::setCallback(update);
	
	startTime = System::time();
	
	Keyboard::the()->KeyDown = keyDown;
	Keyboard::the()->KeyUp = keyUp;
	Mouse::the()->Move = mouseMove;
	Mouse::the()->Press = mousePress;
	Mouse::the()->Release = mouseRelease;
	
	// Sound initiation
	Audio1::init();
	Audio2::init();
	startRecordingSound = new Sound("sound/start.wav");
	stopRecordingSound = new Sound("sound/stop.wav");
	
	System::start();
	
	return 0;
}
