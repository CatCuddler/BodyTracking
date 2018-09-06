#include "pch.h"

#include <Kore/IO/FileReader.h>
#include <Kore/Graphics4/Graphics.h>
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

#include <iostream>
#include <fstream>
#include <algorithm>    // std::all_of
#include <string>
#include <vector>

#include "kMeans.h"
#include "kMeans.cpp"
#include "Markov.h"
#include "Markov.cpp"
#include "matrix.h"

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


using namespace std;

using namespace Kore;
using namespace Kore::Graphics4;

struct EndEffector {
	Quaternion offsetRotation;
	vec3 offsetPosition;
	int boneIndex;
	bool initialized;

	string boneName;

	EndEffector() : offsetRotation(Quaternion(0, 0, 0, 1)), offsetPosition(vec3(0, 0, 0)), boneIndex(0), initialized(false) {}
};

namespace {
	const int width = 1024;
	const int height = 768;

	Logger* logger;
	int line = 0;

	const int numOfEndEffectors = 5;

	const char* initialTransFilename = "initTransAndRot_1511178843.csv";
	const char* positionDataFilename = "positionData_1511178843.csv";

	const string hmmPath = "../Tracking/";
	const string hmmName = "Yoga_Krieger";

	double startTime;
	double lastTime;

#ifdef KORE_STEAMVR
	// Initial tracked position as base for rotation of any futher data points
	double startX;
	double startZ;
	double startRotCos;
	double startRotSin;
	double transitionX;
	double transitionY;
	double transitionZ;
	int dataPointNumber; // x-th point of data collected in current recording/recognition
#endif

	// swapped to namespace to be accessible to adjust y tracking
	float currentUserHeight;

	// audio cues for Markov
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
	bool Z, X = false;
	bool record = false;
	bool recognition = false;

	// List of identifiers for sensors;  those will be used as a prefix for every row according to which sensor is being recorded. Unused sensors will not be mentioned
	const string headIdentifier = "HMD";
	const string leftIdentifier = "lhC";
	const string rightIdentifier = "rhC";
	const string leftFootIdentifier = "lfT";
	const string rightFootIdentifier = "rfT";
	const string backIdentifier = "bac";
	
	// Vector of data points logged in real time movement recognition
	vector<vector<Point>> recognitionPoints(6);

	Quaternion cameraRotation = Quaternion(0, 0, 0, 1);
	vec3 cameraPosition = vec3(0, 0, 0);

	// Null terminated array of MeshObject pointers
	MeshObject* cubes[] = { nullptr, nullptr, nullptr, nullptr, nullptr };
	Avatar* avatar;
	LivingRoom* livingRoom;

#ifdef KORE_STEAMVR
	int leftTrackerIndex = -1;
	int rightTrackerIndex = -1;
	int leftFootTrackerIndex = -1;
	int rightFootTrackerIndex = -1;
	int backTrackerIndex = -1;
	int hmdIndex = -1; // HMD index to be able to record its movement
#endif

	vec3 desPosition[numOfEndEffectors];
	Quaternion desRotation[numOfEndEffectors];

	//used for tracking the head movement
	vec3 hmdPosition;
	Quaternion hmdRotation;

	// End-effectors
	EndEffector* leftHand;
	EndEffector* rightHand;
	EndEffector* leftFoot;
	EndEffector* rightFoot;
	EndEffector* back;

	mat4 initTrans = mat4::Identity();
	mat4 initTransInv = mat4::Identity();
	const mat4 hmdOffset = mat4::Translation(0, 0.2f, 0);
	Quaternion initRot = Quaternion(0, 0, 0, 1);
	Quaternion initRotInv = Quaternion(0, 0, 0, 1);

	bool initCharacter = false;

	const int leftHandBoneIndex = 10;
	const int rightHandBoneIndex = 29;
	const int leftFootBoneIndex = 49;
	const int rightFootBoneIndex = 53;
	const int backBoneIndex = 4;//3;
	const int renderTrackerOrTargetPosition = 1;		// 0 - dont render, 1 - render desired position

	void renderTracker() {
		switch (renderTrackerOrTargetPosition) {
		case 0:
			// Dont render
			break;
		case 1:
			// Render desired position
			for (int i = 0; i < numOfEndEffectors; ++i) {
				if (cubes[i] != nullptr) {
					cubes[i]->M = mat4::Translation(desPosition[i].x(), desPosition[i].y(), desPosition[i].z()) * desRotation[i].matrix().Transpose();
					Graphics4::setMatrix(mLocation, cubes[i]->M);
					cubes[i]->render(tex);
				}
			}
			break;
		default:
			break;
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

	bool initEndEffector(EndEffector* endEffector, const int boneIndex) {
		if (!endEffector->initialized) {

			endEffector->boneIndex = boneIndex;
			endEffector->offsetRotation = Quaternion(0, 0, 0, 1);
			endEffector->offsetPosition = vec3(0, 0, 0);
	
			if (boneIndex == backBoneIndex) {
				endEffector->offsetPosition = vec3(0, 0.05f, 0);
				endEffector->offsetRotation.rotate(Quaternion(vec3(1, 0, 0), Kore::pi * 1.57)); //red
				endEffector->offsetRotation.rotate(Quaternion(vec3(0, 1, 0), Kore::pi));
				endEffector->boneName = backIdentifier;
			}
			else if (boneIndex == leftHandBoneIndex) {
				endEffector->offsetPosition = vec3(0.02f, 0.02f, 0);
				endEffector->offsetRotation.rotate(Quaternion(vec3(1, 0, 0), -Kore::pi / 1.5f)); //red
				endEffector->offsetRotation.rotate(Quaternion(vec3(0, 1, 0), Kore::pi * 0.1)); //green
				endEffector->boneName = leftIdentifier;
			}
			else if (boneIndex == rightHandBoneIndex) {
				endEffector->offsetPosition = vec3(-0.02f, 0.02f, 0);
				endEffector->offsetRotation.rotate(Quaternion(vec3(1, 0, 0), -Kore::pi / 1.5f)); //red
				endEffector->offsetRotation.rotate(Quaternion(vec3(0, 1, 0), -Kore::pi * 0.1)); //green
				endEffector->boneName = rightIdentifier;
			}
			else if (boneIndex == leftFootBoneIndex) {
				endEffector->offsetPosition = vec3(0, 0, 0.05f);

				endEffector->offsetRotation.rotate(Quaternion(vec3(0, 1, 0), (Kore::pi * 0.5f) + (Kore::pi * 0.0f))); //green
				endEffector->offsetRotation.rotate(Quaternion(vec3(0, 0, 1), (Kore::pi * 0.0f) + (Kore::pi * 0.125f))); //blue
				endEffector->offsetRotation.rotate(Quaternion(vec3(1, 0, 0), (Kore::pi * 0.5f) + (Kore::pi * 0.15f))); //red  //0.16f
				endEffector->boneName = leftFootIdentifier;
			}
			else if (boneIndex == rightFootBoneIndex) {
				endEffector->offsetPosition = vec3(0, 0, 0.05f);

				endEffector->offsetRotation.rotate(Quaternion(vec3(0, 1, 0), (Kore::pi * -0.5f) + (Kore::pi * 0.0f))); //green
				endEffector->offsetRotation.rotate(Quaternion(vec3(0, 0, 1), (Kore::pi * 0.0f) + (Kore::pi * -0.125f))); //blue
				endEffector->offsetRotation.rotate(Quaternion(vec3(1, 0, 0), (Kore::pi * 0.5f) + (Kore::pi * 0.15f))); //red  //0.16f
				endEffector->boneName = rightIdentifier;
			}

			endEffector->initialized = true;
			return true;
		}

		return false;
	}

	void applyOffset(EndEffector* endEffector, Kore::vec3& desPosition, Kore::Quaternion& desRotation) {
		vec3 offsetPosition = endEffector->offsetPosition;
		Quaternion offsetRotation = endEffector->offsetRotation;

		// Apply offset position and rotation
		desRotation.rotate(offsetRotation);
		Kore::mat4 curPos = mat4::Translation(desPosition.x(), desPosition.y(), desPosition.z()) * desRotation.matrix().Transpose() * mat4::Translation(offsetPosition.x(), offsetPosition.y(), offsetPosition.z());
		Kore::vec4 desPos = curPos * vec4(0, 0, 0, 1);
		desPosition = vec3(desPos.x(), desPos.y(), desPos.z());
	}

	void setDesiredPosition(EndEffector* endEffector, Kore::vec3& desPosition, Kore::Quaternion& desRotation) {
		applyOffset(endEffector, desPosition, desRotation);

		// Transform desired position to the character coordinate system
		vec4 finalPos = initTransInv * vec4(desPosition.x(), desPosition.y(), desPosition.z(), 1);
		avatar->setDesiredPosition(endEffector->boneIndex, finalPos);
	}


	// desPosition and desRotation are global
	void setDesiredPositionAndOrientation(EndEffector* endEffector, Kore::vec3& desPosition, Kore::Quaternion& desRotation) {
		applyOffset(endEffector, desPosition, desRotation);

		// Transform desired position to the character local coordinate system
		vec4 finalPos = initTransInv * vec4(desPosition.x(), desPosition.y(), desPosition.z(), 1);
		Kore::Quaternion finalRot = initRotInv.rotated(desRotation);

		avatar->setDesiredPositionAndOrientation(endEffector->boneIndex, finalPos, finalRot);
	}

	void setBackBonePosition(Kore::vec3& desPosition, Kore::Quaternion& desRotation) {
		applyOffset(back, desPosition, desRotation);

		initRot = desRotation;
		initRot.normalize();
		initRotInv = initRot.invert();
		avatar->M = mat4::Translation(desPosition.x(), 0, desPosition.z()) * initRot.matrix().Transpose();
		initTransInv = avatar->M.Invert();
	}

	// record the current position and rotation of a tracker - works for controllers, trackers and HMD - and save it into a file
#ifdef KORE_STEAMVR
	void recordMovement(string identifier, int currentIndex = 0) {
		if (record || recognition) { // either recording or recognition is active

			// check for HMD as it is not part of desPosition/desRotation array
			if (identifier == headIdentifier) {
				transitionX = hmdPosition.x() - startX;
				transitionY = hmdPosition.y();
				transitionZ = hmdPosition.z() - startZ;
			}
			// check for index based position in array of desRotation and desPosition. Only controllers and trackers are part of this array
			else {
				transitionX = desPosition[currentIndex].x() - startX;
				transitionY = desPosition[currentIndex].y();
				transitionZ = desPosition[currentIndex].z() - startZ;
			}

			if (record) { // data is recorded

				vec3 hmmPos((transitionX * startRotCos - transitionZ * startRotSin), ((transitionY / currentUserHeight) * 1.8), (transitionZ * startRotCos + transitionX * startRotSin));
				// TODO: why dont we use raw data?
				logger->saveData(lastTime, identifier, hmmPos);
			}

			if (recognition) { // data is stored internally for evaluation at the end of recognition

				double x, y, z;
				x = (transitionX * startRotCos - transitionZ * startRotSin);
				y = (transitionY / currentUserHeight) * 1.8;
				z = (transitionZ * startRotCos + transitionX * startRotSin);

				vector<double> values = { x, y, z };
				Point point = Point(dataPointNumber, values);
				dataPointNumber++;

				if (identifier.compare(headIdentifier) == 0)			recognitionPoints.at(0).push_back(point);
				else if (identifier.compare(leftIdentifier) == 0)		recognitionPoints.at(1).push_back(point);
				else if (identifier.compare(rightIdentifier) == 0)		recognitionPoints.at(2).push_back(point);
				else if (identifier.compare(backIdentifier) == 0)		recognitionPoints.at(3).push_back(point);
				else if (identifier.compare(leftFootIdentifier) == 0)	recognitionPoints.at(4).push_back(point);
				else if (identifier.compare(rightFootIdentifier) == 0)	recognitionPoints.at(5).push_back(point);
			}

		}
	}

	void gamepadButton(int buttonNr, float value);
#endif
	
	void update() {
		float t = (float)(System::time() - startTime);
		double deltaT = t - lastTime;
		lastTime = t;

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
			float currentAvatarHeight = avatar->getHeight();

			state = VrInterface::getSensorState(0);
			vec3 hmdPos = state.pose.vrPose.position; // z -> face, y -> up down
			currentUserHeight = hmdPos.y();

			float scale = currentUserHeight / currentAvatarHeight;
			avatar->setScale(scale);

			// Set initial transformation
			initTrans = mat4::Translation(hmdPos.x(), 0, hmdPos.z());

			// Set initial orientation
			Quaternion hmdOrient = state.pose.vrPose.orientation;
			float zAngle = 2 * Kore::acos(hmdOrient.y);
			initRot.rotate(Quaternion(vec3(0, 0, 1), -zAngle));

			initRotInv = initRot.invert();

			avatar->M = initTrans * initRot.matrix().Transpose() * hmdOffset;
			initTransInv = (initTrans * initRot.matrix().Transpose() * hmdOffset).Invert();

			log(Info, "current avatar height %f, current user height %f, scale %f", currentAvatarHeight, currentUserHeight, scale);

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
					//leftTrackerIndex = -1;
					//rightTrackerIndex = i;
				}
				else if (controller.trackedDevice == TrackedDevice::Controller) {
					//Hand tracker
					vec3 trackerPos = controller.vrPose.position;
					vec4 trackerTransPos = initTransInv * vec4(trackerPos.x(), trackerPos.y(), trackerPos.z(), 1);
					if (trackerTransPos.x() > 0) {
						log(Info, "leftTrackerIndex: %i -> %i", leftTrackerIndex, i);
						leftTrackerIndex = i;
					}
					else {
						log(Info, "rightTrackerIndex: %i -> %i", rightTrackerIndex, i);
						rightTrackerIndex = i;
					}
					
					Kore::Gamepad::get(i)->Button = gamepadButton;
				}
				else if (controller.trackedDevice == TrackedDevice::HMD) {
					//Head mounted display
					log(Info, "hmdIndex: %i -> %i", hmdIndex, i);
					hmdIndex = i;
				}
			}

			if (record) {
				vec4 initPos = initTrans * vec4(0, 0, 0, 1);
				logger->saveInitTransAndRot(vec3(initPos.x(), initPos.y(), initPos.z()), initRot);
			}

			initCharacter = true;
		}

		VrPoseState controller;
		if (hmdIndex != -1) {
			SensorState state = VrInterface::getSensorState(0);
			controller = VrInterface::getController(hmdIndex);

			// Get controller position and rotation
			hmdPosition = state.pose.vrPose.position;
			hmdRotation = state.pose.vrPose.orientation;

			// print controller position to file (if recording is enabled)
			recordMovement(headIdentifier);

			// currently this is only recorded and not used in any way
		}

		if (leftTrackerIndex != -1) {
			controller = VrInterface::getController(leftTrackerIndex);

			// Get controller position and rotation
			desPosition[0] = controller.vrPose.position;
			desRotation[0] = controller.vrPose.orientation;

			// print controller position to file (if recording is enabled)
			recordMovement(leftIdentifier, 0);

			//setDesiredPositionAndOrientation(leftHand, desPosition[0], desRotation[0]);
		}

		if (rightTrackerIndex != -1) {
			controller = VrInterface::getController(rightTrackerIndex);

			// Get controller position and rotation
			desPosition[1] = controller.vrPose.position;
			desRotation[1] = controller.vrPose.orientation;

			// print controller position to file (if recording is enabled)
			recordMovement(rightIdentifier, 1);

			//setDesiredPositionAndOrientation(rightHand, desPosition[1], desRotation[1]);
		}

		if (backTrackerIndex != -1) {
			controller = VrInterface::getController(backTrackerIndex);

			// Get controller position and rotation
			desPosition[2] = controller.vrPose.position;
			desRotation[2] = controller.vrPose.orientation;

			// print controller position to file (if recording is enabled)
			recordMovement(backIdentifier, 2);


			//setBackBonePosition(desPosition[2], desRotation[2]);
		}

		if (leftFootTrackerIndex != -1) {
			controller = VrInterface::getController(leftFootTrackerIndex);

			// Get controller position and rotation
			desPosition[3] = controller.vrPose.position;
			desRotation[3] = controller.vrPose.orientation;

			// print controller position to file (if recording is enabled)
			recordMovement(leftFootIdentifier, 3);

			//setDesiredPositionAndOrientation(leftFoot, desPosition[3], desRotation[3]);

			//if (track == 0) {
			//	setDesiredPositionAndOrientation(leftHand, desPosition[0], desRotation[0]);
			//}
			//else if (track == 1) {
			//	setDesiredPosition(leftFoot, desPosition[0], desRotation[0]);
			//}
		}

		if (rightFootTrackerIndex != -1) {
			controller = VrInterface::getController(rightFootTrackerIndex);

			// Get controller position and rotation
			desPosition[4] = controller.vrPose.position;
			desRotation[4] = controller.vrPose.orientation;

			// print controller position to file (if recording is enabled)
			recordMovement(rightFootIdentifier, 4);

			//setDesiredPositionAndOrientation(rightFoot, desPosition[4], desRotation[4]);

			//if (track == 0) {
			//	setDesiredPositionAndOrientation(rightHand, desPosition[0], desRotation[0]);
			//}
			//else if (track == 1) {
			//	setDesiredPosition(rightFoot, desPosition[0], desRotation[0]);
			//}
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
			Graphics4::setMatrix(mLocation, avatar->M);
//			avatar->animate(tex, deltaT);

			// Render tracker
			renderTracker();

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
		Graphics4::setMatrix(mLocation, avatar->M);

		// Animate avatar
		//avatar->animate(tex, deltaT);
		//avatar->drawJoints(avatar->M, state.pose.vrPose.eye, state.pose.vrPose.projection, width, height, true);

		// Render tracker
		renderTracker();

		// Render living room
		if (!firstPersonMonitor) {
			renderLivingRoom(V, P);
		}
		else {
			renderLivingRoom(state.pose.vrPose.eye, state.pose.vrPose.projection);
		}

#else
		if (!initCharacter) {
			//avatar->setScale(0.95);	// Scale test

			log(Info, "Read data from file %s", initialTransFilename);
			vec3 initPos = vec3(0, 0, 0);
			logger->readInitTransAndRot(initialTransFilename, &initPos, &initRot);

			initRot.normalize();
			initRotInv = initRot.invert();

			initTrans = mat4::Translation(initPos.x(), initPos.y(), initPos.z()) * initRot.matrix().Transpose();
			avatar->M = initTrans;
			initTransInv = initTrans.Invert();

			initCharacter = true;
		}
		
		const int numOfEndEffectorsToRead = 5;	// TODO: record new data (with HMD) and change to 6
		vec3 rawPos[numOfEndEffectorsToRead];
		Quaternion rawRot[numOfEndEffectorsToRead];
		// Read line
		if (logger->readData(line, numOfEndEffectorsToRead, positionDataFilename, rawPos, rawRot)) {

			for (int tracker = 0; tracker < numOfEndEffectorsToRead; ++tracker) {
			
				desPosition[tracker] = rawPos[tracker];
				desRotation[tracker] = rawRot[tracker];

				bool leftFootTracker = false;
				bool rightFootTracker = false;
				bool leftHandTracker = false;
				bool rightHandTracker = false;
				bool backTracker = false;

				switch (tracker) {
					case 0: leftHandTracker = true;
						break;
					case 1: rightHandTracker = true;
						break;
					case 2: backTracker = true;
						break;
					case 3: leftFootTracker = true;
						break;
					case 4: rightFootTracker = true;
						break;
				}
				

				if (leftFootTracker) {
					setDesiredPositionAndOrientation(leftFoot, desPosition[tracker], desRotation[tracker]);
				}
				else if (rightFootTracker) {
					setDesiredPositionAndOrientation(rightFoot, desPosition[tracker], desRotation[tracker]);
				}
				else if (backTracker) {
					setBackBonePosition(desPosition[tracker], desRotation[tracker]);
				}
				else if (leftHandTracker) {
					setDesiredPositionAndOrientation(leftHand, desPosition[tracker], desRotation[tracker]);
				}
				else if (rightHandTracker) {
					setDesiredPositionAndOrientation(rightHand, desPosition[tracker], desRotation[tracker]);
				}
			}
		}

		//log(Info, "%i", line);
		line += numOfEndEffectorsToRead;

		// Get projection and view matrix
		mat4 P = getProjectionMatrix();
		mat4 V = getViewMatrix();

		Graphics4::setMatrix(vLocation, V);
		Graphics4::setMatrix(pLocation, P);

		Graphics4::setMatrix(mLocation, avatar->M);

		// Animate avatar
		avatar->animate(tex, deltaT);
		//avatar->drawJoints(avatar->M, V, P, width, height, true);

		// Render tracker
		renderTracker();

		// Render living room
		renderLivingRoom(V, P);
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
		case Kore::KeyE:
#ifdef KORE_STEAMVR
			if (!recognition) { // recording is only possible while there is no recognition in progress
				record = !record;

				if (record) {
					log(Info, "Start recording data");
					Audio1::play(startRecordingSound);
					
					startX = hmdPosition.x();
					startZ = hmdPosition.z();
					startRotCos = Kore::cos(hmdRotation.y * Kore::pi);
					startRotSin = Kore::sin(hmdRotation.y * Kore::pi);

				}
				else {
					log(Info, "Stop recording data");
					Audio1::play(stopRecordingSound);

					logger->closeFile();
				}
			}
#endif
			break;

		case Kore::KeyT:
#ifdef KORE_STEAMVR
			if (!record) { // recognition is only possible while there is no recording in progress
				recognition = !recognition;

				if (recognition) { // starting recognition
					Audio1::play(startRecognitionSound);
					log(Info, "Start recognizing data");
					// clear prievously stored points
					recognitionPoints.at(0).clear();
					recognitionPoints.at(1).clear();
					recognitionPoints.at(2).clear();
					recognitionPoints.at(3).clear();
					recognitionPoints.at(4).clear();
					recognitionPoints.at(5).clear();
					dataPointNumber = 0;
					// save current HMD position and rotation for data normalisation
					startX = hmdPosition.x();
					startZ = hmdPosition.z();
					startRotCos = Kore::cos(hmdRotation.y * Kore::pi);
					startRotSin = Kore::sin(hmdRotation.y * Kore::pi);
				} else { // stoping and evaluation recognition
					log(Info, "Stop recognizing data");
					// read clusters for all trackers from file
					bool trackersPresent[6];
					vector<KMeans> kmeanVector(6);
					for (int ii = 0; ii < 6; ii++) {
						try {
							KMeans kmeans(hmmPath, hmmName + "_" + to_string(ii));
							kmeanVector.at(ii) = kmeans;
							trackersPresent[ii] = true;
							log(Info, "find tracking file");
					
						}
						catch (std::invalid_argument) {
							trackersPresent[ii] = false;
							log(Info, "can't find tracker file");
						}
					}
					vector<bool> trackerMovementRecognised(6, true); // store which trackers were recognised as the correct movement
					for (int ii = 0; ii < 6; ii++) { // check all trackers
						if (!recognitionPoints.at(ii).empty() && trackersPresent[ii]) { // make sure the tracker is currently present and there is a HMM for it
							vector<int> clusteredPoints = kmeanVector.at(ii).matchPointsToClusters(normaliseMeasurements(recognitionPoints.at(ii), kmeanVector.at(ii).getAveragePoints())); // clustering data
							HMM model(hmmPath, hmmName + "_" + to_string(ii)); // reading HMM
							trackerMovementRecognised.at(ii) = (model.calculateProbability(clusteredPoints) > model.getProbabilityThreshold() && !std::equal(clusteredPoints.begin() + 1, clusteredPoints.end(), clusteredPoints.begin())); // calculating probability and comparing with probability threshold as well as applying restfix
							logger->analyseHMM(hmmName.c_str(), model.calculateProbability(clusteredPoints), false);
						}
					}
					logger->analyseHMM(hmmName.c_str(), 0, true);

					if (std::all_of(trackerMovementRecognised.begin(), trackerMovementRecognised.end(), [](bool v) { return v; })) { // all (present) trackers were recognised as correct
						Audio1::play(correctSound);
						log(Info, "The movement is correct");
					} else {
						Audio1::play(wrongSound);
						log(Info, "The movement is wrong");
					}
				}
		}
#endif
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
			cameraRotation.rotate(Quaternion(vec3(0, 1, 0), movementX * mouseSensitivity));
			cameraRotation.rotate(Quaternion(vec3(1, 0, 0), movementY * mouseSensitivity));
		}
	}

	void mousePress(int windowId, int button, int x, int y) {
		rotate = true;
	}

	void mouseRelease(int windowId, int button, int x, int y) {
		rotate = false;
	}
	
#ifdef KORE_STEAMVR
	void gamepadButton(int buttonNr, float value) {
		// Menu button
		if (buttonNr == 1) {
			if (value == 1) {
				log(Info, "Menu button pressed");
			} else {
				log(Info, "Menu button released");
			}
			if (!record) { // recognition is only possible while there is no recording in progress
				recognition = !recognition;

				if (recognition) { // starting recognition
					Audio1::play(startRecognitionSound);
					log(Info, "Start recognizing data");
					// clear prievously stored points
					recognitionPoints.at(0).clear();
					recognitionPoints.at(1).clear();
					recognitionPoints.at(2).clear();
					recognitionPoints.at(3).clear();
					recognitionPoints.at(4).clear();
					recognitionPoints.at(5).clear();
					dataPointNumber = 0;
					// save current HMD position and rotation for data normalisation
					startX = hmdPosition.x();
					startZ = hmdPosition.z();
					startRotCos = Kore::cos(hmdRotation.y * Kore::pi);
					startRotSin = Kore::sin(hmdRotation.y * Kore::pi);
				}
				else { // stoping and evaluation recognition
					log(Info, "Stop recognizing data");
					// read clusters for all trackers from file
					bool trackersPresent[6];
					vector<KMeans> kmeanVector(6);
					for (int ii = 0; ii < 6; ii++) {
						try {
							KMeans kmeans(hmmPath, hmmName + "_" + to_string(ii));
							kmeanVector.at(ii) = kmeans;
							trackersPresent[ii] = true;
						}
						catch (std::invalid_argument) {
							trackersPresent[ii] = false;
							log(Info, "can't find tracker file");
						}
					}
					vector<bool> trackerMovementRecognised(6, true); // store which trackers were recognised as the correct movement
					for (int ii = 0; ii < 6; ii++) { // check all trackers
						if (!recognitionPoints.at(ii).empty() && trackersPresent[ii]) { // make sure the tracker is currently present and there is a HMM for it
							vector<int> clusteredPoints = kmeanVector.at(ii).matchPointsToClusters(normaliseMeasurements(recognitionPoints.at(ii), kmeanVector.at(ii).getAveragePoints())); // clustering data
							HMM model(hmmPath, hmmName + "_" + to_string(ii)); // reading HMM
							trackerMovementRecognised.at(ii) = (model.calculateProbability(clusteredPoints) > model.getProbabilityThreshold() && !std::equal(clusteredPoints.begin() + 1, clusteredPoints.end(), clusteredPoints.begin())); // calculating probability and comparing with probability threshold as well as applying restfix
							logger->analyseHMM(hmmName.c_str(), model.calculateProbability(clusteredPoints), false);
						}
					}
					logger->analyseHMM(hmmName.c_str(), 0, true);

					if (std::all_of(trackerMovementRecognised.begin(), trackerMovementRecognised.end(), [](bool v) { return v; })) { // all (present) trackers were recognised as correct
						Audio1::play(correctSound);
						log(Info, "The movement is correct");
					}
					else {
						Audio1::play(wrongSound);
						log(Info, "The movement is wrong");
					}
				}
			}
		}
		
		// Trigger button
		if (buttonNr == 33) {
			// TODO: Dont use keyboard keys to start/stop recording data; use trigger button on the controller
			if (value == 1) {
				log(Info, "Trigger button pressed");
			} else {
				log(Info, "Trigger button released");
			}
			if (!recognition) { // recording is only possible while there is no recognition in progress
				record = !record;

				if (record) {
					log(Info, "Start recording data");
					Audio1::play(startRecordingSound);

					startX = hmdPosition.x();
					startZ = hmdPosition.z();
					startRotCos = Kore::cos(hmdRotation.y * Kore::pi);
					startRotSin = Kore::sin(hmdRotation.y * Kore::pi);

				}
				else {
					log(Info, "Stop recording data");
					Audio1::play(stopRecordingSound);

					logger->closeFile();
				}
			}
		}

		
		// Grip button
		if (buttonNr == 2) {
			if (value == 1) {
				log(Info, "Grip button pressed");
			} else {
				log(Info, "Grip button released");
			}
		}
	}
#endif

	void loadAvatarShader() {
		FileReader vs("shader.vert");
		FileReader fs("shader.frag");
		vertexShader = new Shader(vs.readAll(), vs.size(), VertexShader);
		fragmentShader = new Shader(fs.readAll(), fs.size(), FragmentShader);

		// This defines the structure of your Vertex Buffer
		structure.add("pos", Float3VertexData);
		structure.add("tex", Float2VertexData);
		structure.add("nor", Float3VertexData);

		pipeline = new PipelineState;
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

		pipeline_living_room = new PipelineState;
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
		cameraRotation.rotate(Quaternion(vec3(0, 1, 0), Kore::pi / 2));
		cameraRotation.rotate(Quaternion(vec3(1, 0, 0), -Kore::pi / 6));

		initRot.rotate(Quaternion(vec3(1, 0, 0), -Kore::pi / 2.0));

		for (int i = 0; i < numOfEndEffectors; ++i) {
			cubes[i] = new MeshObject("cube.ogex", "", structure, 0.05);
		}

		loadLivingRoomShader();
		livingRoom = new LivingRoom("sherlock_living_room/sherlock_living_room.ogex", "sherlock_living_room/", structure_living_room, 1);
		Quaternion livingRoomRot = Quaternion(0, 0, 0, 1);
		livingRoomRot.rotate(Quaternion(vec3(1, 0, 0), -Kore::pi / 2.0));
		livingRoomRot.rotate(Quaternion(vec3(0, 0, 1), Kore::pi / 2.0));
		livingRoom->M = mat4::Translation(-0.7, 0, 0) * livingRoomRot.matrix().Transpose();

		logger = new Logger();

		// Initialize enf effectors
		leftHand = new EndEffector();
		initEndEffector(leftHand, leftHandBoneIndex);
		rightHand = new EndEffector();
		initEndEffector(rightHand, rightHandBoneIndex);
		leftFoot = new EndEffector();
		initEndEffector(leftFoot, leftFootBoneIndex);
		rightFoot = new EndEffector();
		initEndEffector(rightFoot, rightFootBoneIndex);
		back = new EndEffector();
		initEndEffector(back, backBoneIndex);

#ifdef KORE_STEAMVR
		VrInterface::init(nullptr, nullptr, nullptr); // TODO: Remove
#endif
	}

}

int kore(int argc, char** argv) {
	System::init("BodyTracking", width, height);

	init();

	System::setCallback(update);

	// Sound initiation
	Audio1::init();
	Audio2::init();
	startRecordingSound = new Sound("sound/start.wav");
	stopRecordingSound = new Sound("sound/stop.wav");
	correctSound = new Sound("sound/correct.wav");
	wrongSound = new Sound("sound/wrong.wav");
	startRecognitionSound = new Sound("sound/start_recognition.wav");

	startTime = System::time();

	Keyboard::the()->KeyDown = keyDown;
	Keyboard::the()->KeyUp = keyUp;
	Mouse::the()->Move = mouseMove;
	Mouse::the()->Press = mousePress;
	Mouse::the()->Release = mouseRelease;

	System::start();

	return 0;
}
