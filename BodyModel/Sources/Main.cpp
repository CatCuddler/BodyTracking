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
#include "HMM.h"
#include "MachineLearningMotionRecognition.h"

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
	const int numOfEndEffectors = 6;
	
	Logger* logger;

	MachineLearningMotionRecognition* motionRecognizer;
	
	HMM* hmm;
	
	double startTime;
	double lastTime;
	
	// Audio cues
	Sound* startRecordingSound;
	Sound* stopRecordingSound;
	Sound* correctSound;
	Sound* wrongSound;
	Sound* startRecognitionSound;
	
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
	
	bool calibratedAvatarScale = false;
	bool calibratedAvatarControllersAndTrackers = false;
	
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
	
	void renderAllVRDevices() {
		Graphics4::setPipeline(pipeline);
	
#ifdef KORE_STEAMVR
		VrPoseState controller;
		for (int i = 0; i < 16; ++i) {
			controller = VrInterface::getController(i);
			
			vec3 pos = controller.vrPose.position;
			Kore::Quaternion rot = controller.vrPose.orientation;
			
			Kore::mat4 M = mat4::Translation(pos.x(), pos.y(), pos.z()) * rot.matrix().Transpose();
			
			Kore::Quaternion yRot(0, 0, 0, 1);
			yRot.rotate(Kore::Quaternion(vec3(0, 1, 0), Kore::pi));
			mat4 zMirror = mat4::Identity();
			zMirror.Set(2, 2 , -1);
			Kore::mat4 mirrorM = zMirror * mat4::Translation(mirrorOver.x(), mirrorOver.y(), mirrorOver.z()) * yRot.matrix().Transpose() * M;
			
			if (controller.trackedDevice == TrackedDevice::ViveTracker) {
				// Render a tracker for both feet and back
				renderVRDevice(0, M);
				renderVRDevice(0, mirrorM);
			} else if (controller.trackedDevice == TrackedDevice::Controller) {
				// Render a controller for both hands
				renderVRDevice(1, M);
				renderVRDevice(1, mirrorM);
			}

			// Render a local coordinate system only if the avatar is not calibrated
			if (!calibratedAvatarControllersAndTrackers) {
				renderVRDevice(2, M);
				renderVRDevice(2, mirrorM);
			}
		}
#else
		for(int i = 0; i < numOfEndEffectors; ++i) {
			Kore::vec3 desPosition = endEffector[i]->getDesPosition();
			Kore::Quaternion desRotation = endEffector[i]->getDesRotation();
			
			Kore::mat4 M = mat4::Translation(desPosition.x(), desPosition.y(), desPosition.z()) * desRotation.matrix().Transpose();
			
			Kore::Quaternion rot(0, 0, 0, 1);
			rot.rotate(Kore::Quaternion(vec3(0, 1, 0), Kore::pi));
			mat4 zMirror = mat4::Identity();
			zMirror.Set(2, 2 , -1);
			Kore::mat4 mirrorM = zMirror * mat4::Translation(mirrorOver.x(), mirrorOver.y(), mirrorOver.z()) * rot.matrix().Transpose() * M;
			
			if (i == hip || i == rightFoot || i == leftFoot) {
				// Render a tracker for both feet and back
				renderVRDevice(0, M);
				renderVRDevice(0, mirrorM);
			} else if (i == rightHand || i == leftHand) {
				// Render a controller for both hands
				renderVRDevice(1, M);
				renderVRDevice(1, mirrorM);
			}
			
			// Render a local coordinate system only if the avatar is not calibrated
			if (!calibratedAvatarControllersAndTrackers) {
				renderVRDevice(2, M);
				renderVRDevice(2, mirrorM);
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
		Kore::Quaternion rot = initRot;
		rot.rotate(Kore::Quaternion(vec3(0, 0, 1), Kore::pi));
		mat4 mirrorMatrix = mat4::Identity();
		mirrorMatrix.Set(2, 2 , -1);
		mat4 initTransMirror = mirrorMatrix * mat4::Translation(mirrorOver.x(), mirrorOver.y(), mirrorOver.z()) * rot.matrix().Transpose();
		
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
	

	Kore::Quaternion toQuaternion(vec3 base)
	{
		// Abbreviations for the various angular functions
		double cy = cos(base.z() * 0.5);
		double sy = sin(base.z() * 0.5);
		double cp = cos(base.y() * 0.5);
		double sp = sin(base.y() * 0.5);
		double cr = cos(base.x() * 0.5);
		double sr = sin(base.x() * 0.5);

		Kore::Quaternion q;

		q.w = cy * cp * cr + sy * sp * sr;
		q.x = cy * cp * sr - sy * sp * cr;
		q.y = sy * cp * sr + cy * sp * cr;
		q.z = sy * cp * cr - cy * sp * sr;

		return q;
	}


	void executeMovement(int endEffectorID) {
		Kore::vec3 desPosition = endEffector[endEffectorID]->getDesPosition();
		Kore::Quaternion desRotation = endEffector[endEffectorID]->getDesRotation();

		// Save raw data
		if (logRawData) logger->saveData(endEffector[endEffectorID]->getName(), desPosition, desRotation, avatar->scale);
		
		// Save data to either train hmm or to recognize a movement
		//if (hmm->hmmActive()) hmm->recordMovement(lastTime, endEffector[endEffectorID]->getName(), desPosition, desRotation);
		if (hmm->hmmRecognizing()) hmm->recordMovement(lastTime, endEffector[endEffectorID]->getName(), desPosition, desRotation);
		
		if (calibratedAvatarControllersAndTrackers) {
			// Transform desired position/rotation to the character local coordinate system
			desPosition = initTransInv * vec4(desPosition.x(), desPosition.y(), desPosition.z(), 1);
			desRotation = initRotInv.rotated(desRotation);
			
			// Add offset
			Kore::Quaternion offsetRotation = endEffector[endEffectorID]->getOffsetRotation();
			vec3 offserPosition = endEffector[endEffectorID]->getOffsetPosition();
			Kore::Quaternion finalRot = desRotation.rotated(offsetRotation);
			vec3 finalPos = mat4::Translation(desPosition.x(), desPosition.y(), desPosition.z()) * finalRot.matrix().Transpose() * mat4::Translation(offserPosition.x(), offserPosition.y(), offserPosition.z()) * vec4(0, 0, 0, 1);
			
			if (endEffectorID == hip) {
				avatar->setFixedPositionAndOrientation(endEffector[endEffectorID]->getBoneIndex(), finalPos, finalRot);
			} else {
				avatar->setDesiredPositionAndOrientation(endEffector[endEffectorID]->getBoneIndex(), endEffector[endEffectorID]->getIKMode(), finalPos, finalRot);
			}
			
			// TODO: -replace placholder vectors with actual data, once integrated in endEffector API
			// TODO: -add additional sensor types, name and calibrate them
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

				
				#ifdef KORE_STEAMVR

					VrPoseState sensorState;
					// The HMD is accessed slightly different from controllers and trackers
					if (endEffectorID == head) {
						sensorState = VrInterface::getSensorState(0).pose;
					}
					else {
						sensorState = VrInterface::getController(endEffector[endEffectorID]->getDeviceIndex());
					}

					rawLinVel = sensorState.linearVelocity;
					rawAngVel = sensorState.angularVelocity;

					desLinVel = initTransInv * vec4(rawLinVel.x(), rawLinVel.y(), rawLinVel.z(), 1);
					
					//TODO: - check toQuaternion function, and whether this produces the desired result
					Kore::Quaternion rawAngVelQuat = toQuaternion(rawAngVel);
					desAngVel = initRotInv.rotated(rawAngVelQuat);

					rawPosition = sensorState.vrPose.position;
					rawRotation = sensorState.vrPose.orientation;
					

					Kore::log(Kore::LogLevel::Info, "sensor: (%s)", endEffector[endEffectorID]->getName());
					Kore::log(Kore::LogLevel::Info, "linVel: (%f, %f, %f)", rawLinVel.x(), rawLinVel.y(), rawLinVel.z());
					Kore::log(Kore::LogLevel::Info, "angVel: (%f, %f, %f)", rawAngVel.x(), rawAngVel.y(), rawAngVel.z());

				#else
					// these placeholder values are only meant for testing with predetermined movement sets, not for recording new data
					rawAngVel = vec3(1, 2, 3);
					desAngVel = rawAngVel;
					rawLinVel = vec3(7, 8, 9);
					desLinVel = rawLinVel;
					rawPosition = desPosition;
					rawRotation = desRotation;
				#endif

				motionRecognizer->processMovementData(
					endEffector[endEffectorID]->getName(), 
					rawPosition, desPosition, finalPos, 
					rawRotation, desRotation, finalRot, 
					rawAngVel, desAngVel, 
					rawLinVel, desLinVel, 
					avatar->scale, lastTime);
			}

			if (hmm->hmmRecording()) hmm->recordMovement(lastTime, endEffector[endEffectorID]->getName(), finalPos, finalRot);
		}
	}

	
	void calibrate() {
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
		
		if (!logRawData && !motionRecognizer->isActive() && !hmm->isRecordingActive() && !hmm->isRecognitionActive()) {
			Audio1::play(startRecordingSound);
			logger->startLogger("logData");
		}
		else if (logRawData && !motionRecognizer->isActive() && !hmm->isRecordingActive() && !hmm->isRecognitionActive()) {
			Audio1::play(stopRecordingSound);
			logger->endLogger();
		}
		
		// HMM
		if(hmm->isRecordingActive()) {
			// Recording a movement
			hmm->recording = !hmm->recording;
			if (hmm->recording) {
				Audio1::play(startRecordingSound);
				hmm->startRecording(endEffector[head]->getDesPosition(), endEffector[head]->getDesRotation());
			} else {
				Audio1::play(stopRecordingSound);
				hmm->stopRecording();
			}
		} else if(hmm->isRecognitionActive()) {
			// Recognizing a movement
			hmm->recognizing = !hmm->recognizing;
			if (hmm->recognizing) {
				Audio1::play(startRecognitionSound);
				log(Info, "Start recognizing the motion");
				hmm->startRecognition(endEffector[head]->getDesPosition(), endEffector[head]->getDesRotation());
			} else {
				log(Info, "Stop recognizing the motion");
				bool correct = hmm->stopRecognition();
				if (correct) {
					log(Info, "The movement is correct!");
					Audio1::play(correctSound);
				} else {
					log(Info, "The movement is wrong");
					Audio1::play(wrongSound);
				}
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
		
		log(Info, "%s: %i -> %i", endEffector[efID]->getName(), endEffector[efID]->getDeviceIndex(), deviceID);
	}
	
	void assignControllerAndTracker() {
		VrPoseState vrDevice;
		
		// Get indices for VR devices
		for (int i = 0; i < 16; ++i) {
			vrDevice = VrInterface::getController(i);
			
			vec3 devicePos = vrDevice.vrPose.position;
			Kore::Quaternion deviceRot = vrDevice.vrPose.orientation;
			
			// Transform desired position to the character local coordinate system
			vec4 deviceTransPos = initTransInv * vec4(devicePos.x(), devicePos.y(), devicePos.z(), 1);
			
			if (vrDevice.trackedDevice == TrackedDevice::ViveTracker) {
				if (devicePos.y() < currentUserHeight / 3) {
					// Foot tracker
					if (deviceTransPos.x() > 0) {
						initEndEffector(leftFoot, i, devicePos, deviceRot);
						log(Info, "leftFoot: %i -> %i", endEffector[leftFoot]->getDeviceIndex(), i);
					} else {
						initEndEffector(rightFoot, i, devicePos, deviceRot);
						log(Info, "rightFoot: %i -> %i", endEffector[rightFoot]->getDeviceIndex(), i);
					}
				} else {
					// Hip tracker
					initEndEffector(hip, i, devicePos, deviceRot);
					log(Info, "hip: %i -> %i", endEffector[hip]->getDeviceIndex(), i);
				}
			} else if (vrDevice.trackedDevice == TrackedDevice::Controller) {
				// Hand controller
				if (deviceTransPos.x() > 0) {
					initEndEffector(leftHand, i, devicePos, deviceRot);
					log(Info, "leftHand: %i -> %i", endEffector[leftHand]->getDeviceIndex(), i);
				} else {
					initEndEffector(rightHand, i, devicePos, deviceRot);
					log(Info, "rightHand: %i -> %i", endEffector[rightHand]->getDeviceIndex(), i);
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

		// Grip button => set size and reset an avatar to a default T-Pose
		if (buttonNr == 2 && value == 1) {
			calibratedAvatarControllersAndTrackers = false;
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
		
		for (int i = 0; i < 16; ++i) {
			controller = VrInterface::getController(i);
			
			if (controller.trackedDevice == TrackedDevice::Controller)
				Gamepad::get(i)->Button = gamepadButton;
		}
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
		if (currentFile < numFiles && logger->readData(numOfEndEffectors, files[currentFile], desPosition, desRotation, scaleFactor)) {
			for (int i = 0; i < numOfEndEffectors; ++i) {
				endEffector[i]->setDesPosition(desPosition[i]);
				endEffector[i]->setDesRotation(desRotation[i]);
			}
			
			if (!calibratedAvatarControllersAndTrackers) {
				avatar->resetPositionAndRotation();
				avatar->setScale(scaleFactor);
				calibratedAvatarScale = true;
				calibrate();
				calibratedAvatarControllersAndTrackers = true;
			}
			
			for (int i = 0; i < numOfEndEffectors; ++i) executeMovement(i);
			
		} else {
			if (eval) {
				if (loop >= 0) {
					logger->saveEvaluationData(avatar);
					// log(Kore::Info, "%i more iterations!", loop);
					log(Kore::Info, "%s\t%i\t%f", files[currentFile], ikMode, evalValue[ikMode]);
					loop--;
					
					if (loop < 0) {
						logger->endEvaluationLogger();
						
						if (currentFile >= evalFilesInGroup - 1 && ikMode >= evalMaxIk && evalSteps <= 1)
							exit(0);
						else {
							if (evalSteps <= 1) {
								evalValue[ikMode] = evalInitValue[ikMode];
								evalSteps = evalStepsInit;
								ikMode++;
								endEffector[head]->setIKMode((IKMode)ikMode);
								endEffector[leftHand]->setIKMode((IKMode)ikMode);
								endEffector[rightHand]->setIKMode((IKMode)ikMode);
								endEffector[leftFoot]->setIKMode((IKMode)ikMode);
								endEffector[rightFoot]->setIKMode((IKMode)ikMode);
								endEffector[hip]->setIKMode((IKMode)ikMode);
							} else {
								evalValue[ikMode] += evalStep;
								evalSteps--;
							}
							
							if (ikMode > evalMaxIk) {
								ikMode = evalMinIk;
								currentFile++;
							}
							
							loop = 0;
							logger->startEvaluationLogger(files[currentFile], ikMode, lambda[ikMode], errorMaxPos[ikMode], errorMaxRot[ikMode], maxSteps[ikMode]);
						}
					}
				}
			} else {
				currentFile++;
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

	bool calibratedAvatarCompletely() {
		return (calibratedAvatarScale && calibratedAvatarControllersAndTrackers);
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
			default:
				break;
		}


		// the motion recognizer handles its own input in order to start recording data for specific tasks
		// uses the numpad and space bar (if the class is in use)
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
		
#ifdef KORE_STEAMVR
		avatar = new Avatar("avatar/avatar.ogex", "avatar/", structure);
#else
		avatar = new Avatar("avatar/avatar.ogex", "avatar/", structure);
#endif
		
		initRot = Kore::Quaternion(0, 0, 0, 1);
		initRot.rotate(Kore::Quaternion(vec3(1, 0, 0), -Kore::pi / 2.0));
		initRot.rotate(Kore::Quaternion(vec3(0, 0, 1), Kore::pi / 2.0));
		initRot.normalize();
		initRotInv = initRot.invert();
		
		vec3 initPos = initTrans * vec4(0, 0, 0, 1);
		initTrans = mat4::Translation(initPos.x(), initPos.y(), initPos.z()) * initRot.matrix().Transpose();
		initTransInv = initTrans.Invert();
		
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
		
		hmm = new HMM(*logger);
		
		endEffector = new EndEffector*[numOfEndEffectors];
		endEffector[head] = new EndEffector(headBoneIndex);
		endEffector[hip] = new EndEffector(hipBoneIndex);
		endEffector[leftHand] = new EndEffector(leftHandBoneIndex);
		endEffector[rightHand] = new EndEffector(rightHandBoneIndex);
		endEffector[leftFoot] = new EndEffector(leftFootBoneIndex);
		endEffector[rightFoot] = new EndEffector(rightFootBoneIndex);
		
#ifdef KORE_STEAMVR
		VrInterface::init(nullptr, nullptr, nullptr); // TODO: Remove
#endif
	}
}

int kore(int argc, char** argv) {
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
	correctSound = new Sound("sound/correct.wav");
	wrongSound = new Sound("sound/wrong.wav");
	startRecognitionSound = new Sound("sound/start_recognition.wav");
	
	System::start();
	
	return 0;
}
