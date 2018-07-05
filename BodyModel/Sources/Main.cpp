#include "pch.h"

#include <Kore/IO/FileReader.h>
#include <Kore/Graphics4/Graphics.h>
#include <Kore/Graphics4/PipelineState.h>
#include <Kore/Graphics1/Color.h>
#include <Kore/Input/Keyboard.h>
#include <Kore/Input/Mouse.h>
#include <Kore/System.h>
#include <Kore/Log.h>

#include "MeshObject.h"
#include "Avatar.h"
#include "LivingRoom.h"
#include "RotationUtility.h"
#include "Logger.h"

#ifdef KORE_STEAMVR
#include <Kore/Vr/VrInterface.h>
#include <Kore/Vr/SensorState.h>
#include <Kore/Input/Gamepad.h>
#endif

using namespace Kore;
using namespace Kore::Graphics4;

struct EndEffector {
    Kore::Quaternion offsetRotation;
	vec3 offsetPosition;
	int boneIndex;
	bool initialized;

    EndEffector() : offsetRotation(Kore::Quaternion(0, 0, 0, 1)), offsetPosition(vec3(0, 0, 0)), boneIndex(0), initialized(false) {}
};

namespace {
	const int width = 1024;
	const int height = 768;
	
	Logger* logger;
	bool logData = false;
	int line = 0;

	//tracked data of 5 tracker
	const int numOfEndEffectors = 5;
	const char* initialTransFilename = "initTransAndRot.csv";
	const char* positionDataFilename = "positionData.csv";
	//const char* initialTransFilename = "initTransAndRot_calibration.csv";
	//const char* positionDataFilename = "positionData_calibration.csv";
	
    struct timespec start, end;
	double startTime;
	double lastTime;
	float timer;
  int totalNum;

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
    bool renderRoom = true;
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
	bool Z, X = false;

	Kore::Quaternion cameraRotation = Kore::Quaternion(0, 0, 0, 1);
	vec3 cameraPosition = vec3(0, 0, 0);

	// Null terminated array of MeshObject pointers
	MeshObject* cubes[] = { nullptr, nullptr, nullptr, nullptr, nullptr };
	Avatar* avatar;
	LivingRoom* livingRoom;

#ifdef KORE_STEAMVR
	int leftHandTrackerIndex = -1;
	int rightHandTrackerIndex = -1;
	int leftFootTrackerIndex = -1;
	int rightFootTrackerIndex = -1;
	int backTrackerIndex = -1;

	float currentUserHeight;

	// Buttons
	bool triggerButton;
	bool menuButton;
	bool gripButton;
#endif

	vec3 desPosition[numOfEndEffectors];
	Kore::Quaternion desRotation[numOfEndEffectors];

	// End-effectors
	EndEffector* leftHand;
	EndEffector* rightHand;
	EndEffector* leftFoot;
	EndEffector* rightFoot;
	EndEffector* back;

	mat4 initTrans = mat4::Identity();
	mat4 initTransInv = mat4::Identity();
    Kore::Quaternion initRot;
    Kore::Quaternion initRotInv = Kore::Quaternion(0, 0, 0, 1);

	bool initCharacter = false;

	const bool renderTrackerAndController = true;
	const int leftHandBoneIndex = 16;
	const int rightHandBoneIndex = 26;
	const int leftFootBoneIndex = 6;
	const int rightFootBoneIndex = 31;
    const int backBoneIndex = 8;
	
	void renderTracker() {
		// Render desired position
		for (int i = 0; i < numOfEndEffectors; ++i) {
			if (cubes[i] != nullptr) {
				cubes[i]->M = mat4::Translation(desPosition[i].x(), desPosition[i].y(), desPosition[i].z()) * desRotation[i].matrix().Transpose();
				Graphics4::setMatrix(mLocation, cubes[i]->M);
				cubes[i]->render(tex);
			}
		}
	}

	void renderLivingRoom(mat4 V, mat4 P) {
		Graphics4::setPipeline(pipeline_living_room);

		livingRoom->setLights(lightCount_living_room, lightPosLocation_living_room);
		Graphics4::setMatrix(vLocation_living_room, V);
		Graphics4::setMatrix(pLocation_living_room, P);
		livingRoom->render(tex_living_room, mLocation_living_room, mLocation_living_room_inverse, diffuse_living_room, specular_living_room, specular_power_living_room);
	}

	Kore::mat4 getProjectionMatrix() {
		mat4 P = mat4::Perspective(45, (float)width / (float)height, 0.01f, 1000);
		P.Set(0, 0, -P.get(0, 0));
		return P;
	}

	Kore::mat4 getViewMatrix() {
		vec3 lookAt = cameraPosition + vec3(0, 0, -1);
		mat4 V = mat4::lookAt(cameraPosition, lookAt, vec3(0, 1, 0));
		V *= cameraRotation.matrix();
		return V;
	}

	bool calibrate(EndEffector* endEffector, const int boneIndex) {
		// TODO: calibrate -> should be independent from tracker or controller orientation
		if (!endEffector->initialized) {

			endEffector->boneIndex = boneIndex;
			endEffector->offsetRotation = Kore::Quaternion(0, 0, 0, 1);
			endEffector->offsetPosition = vec3(0, 0, 0);
            
            // x: red, y: green, z: blue

			if (boneIndex == backBoneIndex) {
				endEffector->offsetPosition = vec3(0, 0.05f, 0);
                endEffector->offsetRotation.rotate(Kore::Quaternion(vec3(1, 0, 0), -Kore::pi * 0.92));
				endEffector->offsetRotation.rotate(Kore::Quaternion(vec3(0, 1, 0), Kore::pi));
			}
			else if (boneIndex == leftHandBoneIndex) {
				endEffector->offsetPosition = vec3(0.02f, 0.02f, 0);
				endEffector->offsetRotation.rotate(Kore::Quaternion(vec3(1, 0, 0), -Kore::pi * 0.6f));
				endEffector->offsetRotation.rotate(Kore::Quaternion(vec3(0, 0, 1), -Kore::pi * 0.1f));
				endEffector->offsetRotation.rotate(Kore::Quaternion(vec3(0, 1, 0), Kore::pi * 0.5f));
			}
			else if (boneIndex == rightHandBoneIndex) {
				endEffector->offsetPosition = vec3(-0.02f, 0.02f, 0);
				endEffector->offsetRotation.rotate(Kore::Quaternion(vec3(1, 0, 0), -Kore::pi * 0.6f));
				endEffector->offsetRotation.rotate(Kore::Quaternion(vec3(0, 0, 1), Kore::pi * 0.1f));
				endEffector->offsetRotation.rotate(Kore::Quaternion(vec3(0, 1, 0), -Kore::pi * 0.5f));
			}
			else if (boneIndex == leftFootBoneIndex) {
				endEffector->offsetPosition = vec3(0.05f, 0, 0);
				endEffector->offsetRotation.rotate(Kore::Quaternion(vec3(1, 0, 0), Kore::pi));
				endEffector->offsetRotation.rotate(Kore::Quaternion(vec3(0, 1, 0), -Kore::pi * 0.5f));
				endEffector->offsetRotation.rotate(Kore::Quaternion(vec3(0, 0, 1), -Kore::pi * 0.1f));
			}
			else if (boneIndex == rightFootBoneIndex) {
                endEffector->offsetPosition = vec3(0.05f, 0, 0);
				endEffector->offsetRotation.rotate(Kore::Quaternion(vec3(1, 0, 0), Kore::pi));
				endEffector->offsetRotation.rotate(Kore::Quaternion(vec3(0, 1, 0), Kore::pi * 0.5f));
				endEffector->offsetRotation.rotate(Kore::Quaternion(vec3(0, 0, 1), Kore::pi * 0.1f));
			}
            
			endEffector->offsetRotation.normalize();
			endEffector->initialized = true;
            
			return true;
		}

		return false;
	}

	void applyOffset(EndEffector* endEffector, Kore::vec3& desPosition, Kore::Quaternion& desRotation) {
		vec3 offsetPosition = endEffector->offsetPosition;
		Kore::Quaternion offsetRotation = endEffector->offsetRotation;

		// Apply offset position and rotation
		desRotation.rotate(offsetRotation);
		Kore::mat4 curPos = mat4::Translation(desPosition.x(), desPosition.y(), desPosition.z()) * desRotation.matrix().Transpose() * mat4::Translation(offsetPosition.x(), offsetPosition.y(), offsetPosition.z());
		Kore::vec4 desPos = curPos * vec4(0, 0, 0, 1);
		desPosition = vec3(desPos.x(), desPos.y(), desPos.z());
	}

	// desPosition and desRotation are global
	void setDesiredPositionAndOrientation(EndEffector* endEffector, Kore::vec3& desPosition, Kore::Quaternion& desRotation) {
		if (logData) {
			logger->saveData(desPosition, desRotation);
		}

		applyOffset(endEffector, desPosition, desRotation);

		// Transform desired position to the character local coordinate system
		vec4 finalPos = initTransInv * vec4(desPosition.x(), desPosition.y(), desPosition.z(), 1);
		Kore::Quaternion finalRot = initRotInv.rotated(desRotation);

		avatar->setDesiredPositionAndOrientation(endEffector->boneIndex, finalPos, finalRot);
	}

	void setHipsPosition(Kore::vec3& desPosition) {
        initTrans = mat4::Translation(desPosition.x(), 0, desPosition.z()) * initRot.matrix().Transpose();
		initTransInv = initTrans.Invert();
	}
	
	void readLogData() {
		vec3 rawPos[numOfEndEffectors];
		Kore::Quaternion rawRot[numOfEndEffectors];
		
		// Read line
		if (logger->readData(line, numOfEndEffectors, positionDataFilename, rawPos, rawRot)) {
			
			for (int tracker = 0; tracker < numOfEndEffectors; ++tracker) {
				desPosition[tracker] = rawPos[tracker];
				desRotation[tracker] = rawRot[tracker];
				
				switch (tracker) {
					case 0:
						setDesiredPositionAndOrientation(leftHand, desPosition[tracker], desRotation[tracker]);
						break;
					case 1:
						setDesiredPositionAndOrientation(rightHand, desPosition[tracker], desRotation[tracker]);
						break;
					case 2:
                        setHipsPosition(desPosition[tracker]);
                        setDesiredPositionAndOrientation(back, desPosition[tracker], desRotation[tracker]);
						break;
					case 3:
						setDesiredPositionAndOrientation(leftFoot, desPosition[tracker], desRotation[tracker]);
						break;
					case 4:
						setDesiredPositionAndOrientation(rightFoot, desPosition[tracker], desRotation[tracker]);
						break;
				}
			}
		}
		
		//log(Info, "%i", line);
		line += numOfEndEffectors;
	}


#ifdef KORE_STEAMVR

	void calibrateAvatar();

	void gamepadButton(int buttonNr, float value) {
		// Menu button
		if (buttonNr == 1) {
			if (value == 1) {
				menuButton = true;
				calibrateAvatar();

				log(Info, "Calibrate avatar");
			} else {
				menuButton = false;
			}
      
			/* if (boneIndex != -1) {
				Kore::vec3 desRotEuler = rawRot[i];
				Kore::Quaternion desRot;
				RotationUtility::eulerToQuat(RotationUtility::getRadians(desRotEuler.x()), RotationUtility::getRadians(desRotEuler.y()), RotationUtility::getRadians(desRotEuler.z()), &desRot);
				avatar->setDesiredPositionAndOrientation(boneIndex, vec3(0, 0, 0), desRot);
				
				//vec3 rot = vec3(RotationUtility::getRadians(desRotEuler.x()), RotationUtility::getRadians(desRotEuler.y()), RotationUtility::getRadians(desRotEuler.z()));
				//avatar->setLocalRotation(boneIndex, rot);
      } */
		}

		// Trigger button
		if (buttonNr == 33) {
			if (value == 1) {
				triggerButton = true;
				logData = true;
				log(Info, "Start logging data.");
			}
			else {
				triggerButton = false;
				logData = false;
				log(Info, "Stop recording data.");
			}
		}

		// Grip button
		if (buttonNr == 2) {
			if (value == 1) {
				gripButton = true;
				//log(Info, "Grip Button pressed");
			}
			else {
				gripButton = false;
				//log(Info, "Grip Button unpressed");
			}
		}
	}


	void initController() {
		// Get left and right tracker index
		VrPoseState controller;
		for (int i = 0; i < 16; ++i) {
			controller = VrInterface::getController(i);
			if (controller.trackedDevice == TrackedDevice::ViveTracker) {
				vec3 trackerPos = controller.vrPose.position;
				vec4 trackerTransPos = initTransInv * vec4(trackerPos.x(), trackerPos.y(), trackerPos.z(), 1);
				if (trackerPos.y() < currentUserHeight / 4) {
					//Foot tracker (if y pos is in the lower quarter of the body)
					if (trackerTransPos.x() > 0) {
						log(Info, "leftFootTrackerIndex: %i -> %i", leftFootTrackerIndex, i);
						leftFootTrackerIndex = i;
					}
					else {
						log(Info, "rightFootTrackerIndex: %i -> %i", rightFootTrackerIndex, i);
						rightFootTrackerIndex = i;
					}
				}
				else {
					//back tracker
					log(Info, "backTrackerIndex: %i -> %i", backTrackerIndex, i);
					backTrackerIndex = i;
				}
				//leftHandTrackerIndex = -1;
				//rightHandTrackerIndex = i;
			}
			else if (controller.trackedDevice == TrackedDevice::Controller) {
				//Hand tracker
				vec3 trackerPos = controller.vrPose.position;
				vec4 trackerTransPos = initTransInv * vec4(trackerPos.x(), trackerPos.y(), trackerPos.z(), 1);
				if (trackerTransPos.x() > 0) {
					log(Info, "leftHandTrackerIndex: %i -> %i", leftHandTrackerIndex, i);
					leftHandTrackerIndex = i;
				}
				else {
					log(Info, "rightHandTrackerIndex: %i -> %i", rightHandTrackerIndex, i);
					rightHandTrackerIndex = i;
				}

				//Gamepad::get(i)->Axis = gamepadAxis;
				Gamepad::get(i)->Button = gamepadButton;
			}
		}
	}

	void calibrateAvatar() {
		float currentAvatarHeight = avatar->getHeight();

		SensorState state = VrInterface::getSensorState(0);
		vec3 hmdPos = state.pose.vrPose.position; // z -> face, y -> up down
		currentUserHeight = hmdPos.y();

		float scale = currentUserHeight / currentAvatarHeight;
		avatar->setScale(scale);

		// Set initial transformation
		initTrans = mat4::Translation(hmdPos.x(), 0, hmdPos.z());

		// Set initial orientation
		Quaternion hmdOrient = state.pose.vrPose.orientation;
		float zAngle = 2 * Kore::acos(hmdOrient.y);
		initRot = Quaternion(0, 0, 0, 1);
		initRot.rotate(Quaternion(vec3(1, 0, 0), -Kore::pi / 2.0)); // Make the avatar stand on the feet
		initRot.rotate(Quaternion(vec3(0, 0, 1), -zAngle));

		initRotInv = initRot.invert();

		const mat4 hmdOffset = mat4::Translation(0, 0.2f, 0);
		avatar->M = initTrans * initRot.matrix().Transpose() * hmdOffset;
		initTransInv = (initTrans * initRot.matrix().Transpose() * hmdOffset).Invert();

		log(Info, "current avatar height %f, currend user height %f, scale %f", currentAvatarHeight, currentUserHeight, scale);

		initController();

		if (logData) {
			vec4 initPos = initTrans * vec4(0, 0, 0, 1);
			logger->saveInitTransAndRot(vec3(initPos.x(), initPos.y(), initPos.z()), initRot);
		}
	}

	
#endif

	void update() {
		float t = (float)(System::time() - startTime);
		double deltaT = t - lastTime;
		lastTime = t;

    timer += deltaT;
    // if (timer > 1 && totalNum != avatar->getTotalNum()) {
    if (avatar->getTotalNum() >= 13615 && totalNum != avatar->getTotalNum()) {
        timer = 0;

        clock_gettime(CLOCK_MONOTONIC_RAW, &end);
        uint64_t deltaUs = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;

        totalNum = avatar->getTotalNum();
        float averageIteration = avatar->getAverageIkIteration();
        float averageReached = avatar->getAverageIkReached();
        float averageError = avatar->getAverageIkError();
        float minError = avatar->getMinIkError();
        float maxError = avatar->getMaxIkError();
        float averageTime = (float) deltaUs / (float) totalNum;

        if (logData) logger->saveLogData("it", averageIteration);

        log(Info, "\t\t\tAverage \t\tMin \t\t\tMax \niteration \t%f \nreached \t%f \nerror \t\t%f \t\t%f \t\t%f \ntime \t\t%f \nAfter %i lines and %ius \n", averageIteration, averageReached, averageError, minError, maxError, averageTime, totalNum, deltaUs);
        log(Info, "%f\t%f\t%f\t%f\t%f\t%f\t%i", averageIteration, averageReached, averageError, minError, maxError, averageTime, deltaUs);
		}

		// Move position of camera based on WASD keys, and XZ keys for up and down
		const float moveSpeed = 0.1f;
		if (S) {
			cameraPosition.z() += moveSpeed;
		}
		else if (W) {
			cameraPosition.z() -= moveSpeed;
		}
		if (A) {
			cameraPosition.x() -= moveSpeed;
		}
		else if (D) {
			cameraPosition.x() += moveSpeed;
		}
		if (Z) {
			cameraPosition.y() += moveSpeed;
		}
		else if (X) {
			cameraPosition.y() -= moveSpeed;
		}

		Graphics4::begin();
		Graphics4::clear(Graphics4::ClearColorFlag | Graphics4::ClearDepthFlag, Graphics1::Color::Black, 1.0f, 0);

		Graphics4::setPipeline(pipeline);

#ifdef KORE_STEAMVR

		bool firstPersonVR = true;
		bool firstPersonMonitor = false;

		VrInterface::begin();
		SensorState state;

		if (!initCharacter) {
			/* float currentAvatarHeight = avatar->getHeight();

			state = VrInterface::getSensorState(0);
			vec3 hmdPos = state.pose.vrPose.position; // z -> face, y -> up down
			float currentUserHeight = hmdPos.y();

			float scale = currentUserHeight / currentAvatarHeight;
			avatar->setScale(scale);

			// Set initial transformation
			initTrans = mat4::Translation(hmdPos.x(), 0, hmdPos.z());

			// Set initial orientation
			Kore::Quaternion hmdOrient = state.pose.vrPose.orientation;
			float zAngle = 2 * Kore::acos(hmdOrient.y);
			initRot.rotate(Kore::Quaternion(vec3(0, 0, 1), -zAngle));

			initRotInv = initRot.invert();

			initTrans = initTrans * initRot.matrix().Transpose() * hmdOffset;
			initTransInv = initTrans.Invert();
            
            log(Info, "Numbers of jointDOFs: \t hand: %i, \t foot: %i", avatar->getJointDOFs(leftHand->boneIndex), avatar->getJointDOFs(leftFoot->boneIndex));
			log(Info, "current avatar height %f, currend user height %f, scale %f", currentAvatarHeight, currentUserHeight, scale);

			// Get left and right tracker index
			VrPoseState controller;
			for (int i = 0; i < 16; ++i) {
				controller = VrInterface::getController(i);
				if (controller.trackedDevice == TrackedDevice::ViveTracker) {
					vec3 trackerPos = controller.vrPose.position;
					vec4 trackerTransPos = initTransInv * vec4(trackerPos.x(), trackerPos.y(), trackerPos.z(), 1);
					if (trackerPos.y() < currentUserHeight / 4) {
						//Foot tracker (if y pos is in the lower quarter of the body)
						if (trackerTransPos.x() > 0) {
							log(Info, "leftFootTrackerIndex: %i -> %i", leftFootTrackerIndex, i);
							leftFootTrackerIndex = i;
						}
						else {
							log(Info, "rightFootTrackerIndex: %i -> %i", rightFootTrackerIndex, i);
							rightFootTrackerIndex = i;
						}
					}
					else {
						//back tracker
						log(Info, "backTrackerIndex: %i -> %i", backTrackerIndex, i);
						backTrackerIndex = i;
					}
					//leftHandTrackerIndex = -1;
					//rightHandTrackerIndex = i;
				}
				else if (controller.trackedDevice == TrackedDevice::Controller) {
					//Hand tracker
					vec3 trackerPos = controller.vrPose.position;
					vec4 trackerTransPos = initTransInv * vec4(trackerPos.x(), trackerPos.y(), trackerPos.z(), 1);
					if (trackerTransPos.x() > 0) {
						log(Info, "leftHandTrackerIndex: %i -> %i", leftHandTrackerIndex, i);
						leftHandTrackerIndex = i;
					}
					else {
						log(Info, "rightHandTrackerIndex: %i -> %i", rightHandTrackerIndex, i);
						rightHandTrackerIndex = i;
					}
				}
			}

			if (logData) {
				vec4 initPos = initTrans * vec4(0, 0, 0, 1);
				logger->saveInitTransAndRot(vec3(initPos.x(), initPos.y(), initPos.z()), initRot);
			} */

			calibrateAvatar();
			initCharacter = true;
		}

		VrPoseState controller;
		if (leftHandTrackerIndex != -1) {
			controller = VrInterface::getController(leftHandTrackerIndex);

			// Get controller position and rotation
			desPosition[0] = controller.vrPose.position;
			desRotation[0] = controller.vrPose.orientation;

			setDesiredPositionAndOrientation(leftHand, desPosition[0], desRotation[0]);
		}

		if (rightHandTrackerIndex != -1) {
			controller = VrInterface::getController(rightHandTrackerIndex);

			// Get controller position and rotation
			desPosition[1] = controller.vrPose.position;
			desRotation[1] = controller.vrPose.orientation;

			setDesiredPositionAndOrientation(rightHand, desPosition[1], desRotation[1]);
		}

		if (backTrackerIndex != -1) {
			controller = VrInterface::getController(backTrackerIndex);

			// Get controller position and rotation
			desPosition[2] = controller.vrPose.position;
			desRotation[2] = controller.vrPose.orientation;

			setBackBonePosition(desPosition[2], desRotation[2]);
		}
		
		if (leftFootTrackerIndex != -1) {
			controller = VrInterface::getController(leftFootTrackerIndex);

			// Get controller position and rotation
			desPosition[3] = controller.vrPose.position;
			desRotation[3] = controller.vrPose.orientation;

			setDesiredPositionAndOrientation(leftFoot, desPosition[3], desRotation[3]);
		}

		if (rightFootTrackerIndex != -1) {
			controller = VrInterface::getController(rightFootTrackerIndex);

			// Get controller position and rotation
			desPosition[4] = controller.vrPose.position;
			desRotation[4] = controller.vrPose.orientation;

			setDesiredPositionAndOrientation(rightFoot, desPosition[4], desRotation[4]);
		}

		// Render for both eyes
		for (int eye = 0; eye < 2; ++eye) {
			VrInterface::beginRender(eye);

			Graphics4::setPipeline(pipeline);

			Graphics4::clear(Graphics4::ClearColorFlag | Graphics4::ClearDepthFlag, Graphics1::Color::Black, 1.0f, 0);

			state = VrInterface::getSensorState(eye);
			Graphics4::setMatrix(vLocation, state.pose.vrPose.eye);
			Graphics4::setMatrix(pLocation, state.pose.vrPose.projection);

			// Animate avatar
			Graphics4::setMatrix(mLocation, initTrans);
			avatar->animate(tex, deltaT);

			// Render tracker
			if (renderTrackerAndController) renderTracker();

			// Render living room
			renderLivingRoom(state.pose.vrPose.eye, state.pose.vrPose.projection);

			VrInterface::endRender(eye);
		}

		VrInterface::warpSwap();

		Graphics4::restoreRenderTarget();
		Graphics4::clear(Graphics4::ClearColorFlag | Graphics4::ClearDepthFlag, Graphics1::Color::Black, 1.0f, 0);

		// Render on monitor
		Graphics4::setPipeline(pipeline);

		// Get projection and view matrix
		mat4 P = getProjectionMatrix();
		mat4 V = getViewMatrix();
		if (!firstPersonMonitor) {
			Graphics4::setMatrix(vLocation, V);
			Graphics4::setMatrix(pLocation, P);
		}
		else {
			Graphics4::setMatrix(vLocation, state.pose.vrPose.eye);
			Graphics4::setMatrix(pLocation, state.pose.vrPose.projection);
		}
		Graphics4::setMatrix(mLocation, initTrans);

		// Animate avatar
		avatar->animate(tex, deltaT);

		// Render tracker
		if (renderTrackerAndController) renderTracker();

		// Render living room
		if (renderRoom) {
			if (!firstPersonMonitor) renderLivingRoom(V, P);
			else renderLivingRoom(state.pose.vrPose.eye, state.pose.vrPose.projection);
		}

#else
		if (!initCharacter) {
			vec3 initPos = vec3(0, 0, 0);
      
			log(Info, "Read data from file %s", initialTransFilename);
			logger->readInitTransAndRot(initialTransFilename, &initPos, &initRot);
      
      log(Info, "Numbers of jointDOFs:\n\tback: %i,\n\thand: %i,\n\tfoot: %i", avatar->getJointDOFs(back->boneIndex), avatar->getJointDOFs(leftHand->boneIndex), avatar->getJointDOFs(leftFoot->boneIndex));
			
      initRot.normalize();
			initRotInv = initRot.invert();

			initTrans = mat4::Translation(initPos.x(), initPos.y(), initPos.z()) * initRot.matrix().Transpose();
			initTransInv = initTrans.Invert();

			initCharacter = true;
		}
		
		readLogData();
		
		// Get projection and view matrix
		mat4 P = getProjectionMatrix();
		mat4 V = getViewMatrix();

		Graphics4::setMatrix(vLocation, V);
		Graphics4::setMatrix(pLocation, P);

		Graphics4::setMatrix(mLocation, initTrans);

		// Animate avatar
		avatar->animate(tex, deltaT);

		// Render tracker
		if(renderTrackerAndController) renderTracker();

		// Render living room
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
		case Kore::KeyZ:
			Z = true;
			break;
		case Kore::KeyX:
			X = true;
			break;
		case Kore::KeyR:
#ifdef KORE_STEAMVR
			VrInterface::resetHmdPose();
#endif
			break;
		case KeyL:
			Kore::log(Kore::LogLevel::Info, "Position: (%f, %f, %f)", cameraPosition.x(), cameraPosition.y(), cameraPosition.z());
			Kore::log(Kore::LogLevel::Info, "Rotation: (%f, %f, %f, %f)", cameraRotation.w, cameraRotation.x, cameraRotation.y, cameraRotation.z);
			break;
		case Kore::KeyEscape:
		case KeyQ:
			System::stop();
			break;
		default:
			break;
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
		case Kore::KeyZ:
			Z = false;
			break;
		case Kore::KeyX:
			X = false;
			break;
		default:
			break;
		}
	}

	void mouseMove(int windowId, int x, int y, int movementX, int movementY) {
		if (rotate) {
			const float mouseSensitivity = 0.01f;
			cameraRotation.rotate(Kore::Quaternion(vec3(0, 1, 0), movementX * mouseSensitivity));
			cameraRotation.rotate(Kore::Quaternion(vec3(1, 0, 0), movementY * mouseSensitivity));
		}
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
		// Load shader for living room
		FileReader vs("shader_living_room.vert");
		FileReader fs("shader_living_room.frag");
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
		avatar = new Avatar("avatar/avatar_skeleton_headless.ogex", "avatar/", structure);
#else
		avatar = new Avatar("avatar/avatar_skeleton.ogex", "avatar/", structure);
#endif
		cameraPosition = vec3(-1.1, 1.6, 4.5);
		cameraRotation.rotate(Kore::Quaternion(vec3(0, 1, 0), Kore::pi / 2));
		cameraRotation.rotate(Kore::Quaternion(vec3(1, 0, 0), -Kore::pi / 6));

        initRot = Kore::Quaternion(0, 0, 0, 1);
        initRot.rotate(Kore::Quaternion(vec3(1, 0, 0), -Kore::pi / 2.0));

		for (int i = 0; i < numOfEndEffectors; ++i) {
			cubes[i] = new MeshObject("cube.ogex", "", structure, 0.05);
		}

		if (renderRoom) {
			loadLivingRoomShader();
			livingRoom = new LivingRoom("sherlock_living_room/sherlock_living_room.ogex", "sherlock_living_room/", structure_living_room, 1);
			Kore::Quaternion livingRoomRot = Kore::Quaternion(0, 0, 0, 1);
			livingRoomRot.rotate(Kore::Quaternion(vec3(1, 0, 0), -Kore::pi / 2.0));
			livingRoomRot.rotate(Kore::Quaternion(vec3(0, 0, 1), Kore::pi / 2.0));
			livingRoom->M = mat4::Translation(-0.7, 0, 0) * livingRoomRot.matrix().Transpose();
		}

		logger = new Logger();

		// Initialize end effectors
		leftHand = new EndEffector();
		calibrate(leftHand, leftHandBoneIndex);
		rightHand = new EndEffector();
		calibrate(rightHand, rightHandBoneIndex);
		leftFoot = new EndEffector();
		calibrate(leftFoot, leftFootBoneIndex);
		rightFoot = new EndEffector();
		calibrate(rightFoot, rightFootBoneIndex);
		back = new EndEffector();
		calibrate(back, backBoneIndex);

#ifdef KORE_STEAMVR
		VrInterface::init(nullptr, nullptr, nullptr); // TODO: Remove
#endif
	}

}

int kore(int argc, char** argv) {
	System::init("BodyTracking", width, height);

	init();

	System::setCallback(update);

    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
	startTime = System::time();

	Keyboard::the()->KeyDown = keyDown;
	Keyboard::the()->KeyUp = keyUp;
	Mouse::the()->Move = mouseMove;
	Mouse::the()->Press = mousePress;
	Mouse::the()->Release = mouseRelease;

	System::start();

	return 0;
}
