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
#include "YogaMovement.h"
#include "BinaryTree.h"
#include "Collision.h"

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
	const bool render3DText = true;
	
	EndEffector** endEffector;
	const int numOfEndEffectors = 8;
	
	Logger* logger;
	
	HMM* hmm;
	
	double startTime;
	double lastTime;
	
	BinaryTree* storyLineTree;
	const char* storyLineText;
	
	// Audio cues
	Sound* startRecordingSound;
	Sound* stopRecordingSound;
	Sound* correctSound;
	Sound* wrongSound;
	Sound* startRecognitionSound;
	
	Sound* currentAudio;
	
	// Avatar shader
	VertexStructure structure;
	Shader* vertexShader;
	Shader* fragmentShader;
	PipelineState* pipeline;
	
	TextureUnit tex;
	ConstantLocation pLocation;
	ConstantLocation vLocation;
	ConstantLocation mLocation;
	ConstantLocation cLocation;
	
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
	
	// Null terminated array of 3d text
	MeshObject* textMesh[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
	
	// Avatar
	Avatar* avatar;
	SphereCollider* avatarCollider;
	MeshObject* sphereMesh;
	
	// Virtual environment
	LivingRoom* livingRoom;
	
	// Plattform mesh objects and collider
	MeshObject* plattforms[] = { nullptr, nullptr, nullptr };
	SphereCollider* sphereColliders[] = { nullptr, nullptr, nullptr };
	
	vec3 color0(56.0/255.0, 56.0/255.0, 56.0/255.0);
	vec3 color1(45.0/255.0, 88.0/255.0, 103.0/255.0);
	vec3 color2(30.0/255.0, 46.0/255.0, 77.0/255.0);
	
	Character speakWithCharacter1 = None;
	Character speakWithCharacter2 = None;
	
	// Variables to mirror the room and the avatar
	vec3 mirrorOver(6.057f, 0.0f, 0.04f);
	
	mat4 initTrans;
	mat4 initTransInv;
	Kore::Quaternion initRot;
	Kore::Quaternion initRotInv;
	
	bool calibratedAvatar = false;
	
#ifdef KORE_STEAMVR
	bool controllerButtonsInitialized = false;
	float currentUserHeight;
	bool firstPersonMonitor = false;
#endif
	
	Movement* movement;
	Yoga pose0;
	Yoga pose1;
	
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
		if (!calibratedAvatar) {
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
	
	void renderPlattforms(mat4 V, mat4 P) {
		Graphics4::setPipeline(pipeline);
		
		Graphics4::setMatrix(vLocation, V);
		Graphics4::setMatrix(pLocation, P);
		
		if (pose0 == Yoga0 || pose1 == Yoga0) {
			Graphics4::setMatrix(mLocation, plattforms[0]->M);
			Graphics4::setFloat3(cLocation, color0);
			plattforms[0]->render(tex);

			// Mirror plattform 0
			mat4 plattformTransMirror = getMirrorMatrix() * plattforms[0]->M;
			Graphics4::setMatrix(mLocation, plattformTransMirror);
			plattforms[0]->render(tex);
		}
		
		if (pose0 == Yoga1 || pose1 == Yoga1) {
			Graphics4::setMatrix(mLocation, plattforms[1]->M);
			Graphics4::setFloat3(cLocation, color1);
			plattforms[1]->render(tex);

			// Mirror plattform 1
			mat4 plattformTransMirror = getMirrorMatrix() * plattforms[1]->M;
			Graphics4::setMatrix(mLocation, plattformTransMirror);
			plattforms[1]->render(tex);
		}
		
		if (pose0 == Yoga2 || pose1 == Yoga2) {
			Graphics4::setMatrix(mLocation, plattforms[2]->M);
			Graphics4::setFloat3(cLocation, color2);
			plattforms[2]->render(tex);

			// Mirror plattform 2
			mat4 plattformTransMirror = getMirrorMatrix() * plattforms[2]->M;
			Graphics4::setMatrix(mLocation, plattformTransMirror);
			plattforms[2]->render(tex);
		}
		
		// Reset color
		Graphics4::setFloat3(cLocation, vec3(1.0, 1.0, 1.0));
	}
	
	void render3Dtext(mat4 V, mat4 P) {
		Graphics4::setPipeline(pipeline);
		
		Graphics4::setMatrix(vLocation, V);
		Graphics4::setMatrix(pLocation, P);
		
		Kore::Quaternion textRot = Kore::Quaternion(0, 0, 0, 1);
		textRot.rotate(Kore::Quaternion(vec3(0, 1, 0), -Kore::pi / 2.0));
		
		Kore::mat4 M = Kore::mat4::Identity();
		float yPos = 3.5f;
		
		if (speakWithCharacter1 != None) {
			M = mat4::Translation(2.95f, yPos, -1.5f) * textRot.matrix().Transpose() * mat4::Scale(0.2f, 0.2f, 0.2f);
			Graphics4::setMatrix(mLocation, M);
			vec3 color = vec3(0, 0, 0);
			if (pose0 == Yoga0) color = color0;
			else if (pose0 == Yoga1) color = color1;
			else if (pose0 == Yoga2) color = color2;
			Graphics4::setFloat3(cLocation, color);
			textMesh[speakWithCharacter1]->render(tex);
		}
		
		if (speakWithCharacter2 != None) {
			M = mat4::Translation(2.95f, yPos - 0.2f, -1.5f) * textRot.matrix().Transpose() * mat4::Scale(0.2f, 0.2f, 0.2f);
			Graphics4::setMatrix(mLocation, M);
			vec3 color = vec3(0, 0, 0);
			if (pose1 == Yoga0) color = color0;
			else if (pose1 == Yoga1) color = color1;
			else if (pose1 == Yoga2) color = color2;
			Graphics4::setFloat3(cLocation, color);
			textMesh[speakWithCharacter2]->render(tex);
		}
		
		// Reset color
		Graphics4::setFloat3(cLocation, vec3(1, 1, 1));
		
	}
	
	void renderAvatar(mat4 V, mat4 P) {
		Graphics4::setPipeline(pipeline);
		
		Graphics4::setMatrix(vLocation, V);
		Graphics4::setMatrix(pLocation, P);
		Graphics4::setMatrix(mLocation, initTrans);
		avatar->animate(tex);
		
		// Render collider
		//Graphics4::setMatrix(mLocation, sphereMesh->M);
		//sphereMesh->render(tex);
		
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
	
	void executeMovement(int endEffectorID) {
		Kore::vec3 desPosition = endEffector[endEffectorID]->getDesPosition();
		Kore::Quaternion desRotation = endEffector[endEffectorID]->getDesRotation();

		// Save raw data
		if (logRawData) logger->saveData(endEffector[endEffectorID]->getName(), desPosition, desRotation, avatar->scale);
		
		if (calibratedAvatar) {
			// Transform desired position/rotation to the character local coordinate system
			desPosition = initTransInv * vec4(desPosition.x(), desPosition.y(), desPosition.z(), 1);
			desRotation = initRotInv.rotated(desRotation);

			// Add offset
			Kore::Quaternion offsetRotation = endEffector[endEffectorID]->getOffsetRotation();
			vec3 offsetPosition = endEffector[endEffectorID]->getOffsetPosition();
			Kore::Quaternion finalRot = desRotation.rotated(offsetRotation);
			vec3 finalPos = mat4::Translation(desPosition.x(), desPosition.y(), desPosition.z()) * finalRot.matrix().Transpose() * mat4::Translation(offsetPosition.x(), offsetPosition.y(), offsetPosition.z()) * vec4(0, 0, 0, 1);
			
			if (endEffectorID == hip) {
				avatar->setFixedPositionAndOrientation(endEffector[hip]->getBoneIndex(), finalPos, finalRot);

				// Update the sphere collider for the avatar
				avatarCollider->center = vec3(endEffector[hip]->getDesPosition().x(), 0, endEffector[hip]->getDesPosition().z());
				sphereMesh->M = mat4::Translation(avatarCollider->center.x(), avatarCollider->center.y(), avatarCollider->center.z());

			} else if (endEffectorID == head || endEffectorID == leftForeArm || endEffectorID == rightForeArm || endEffectorID == leftFoot || endEffectorID == rightFoot) {
				avatar->setDesiredPositionAndOrientation(endEffector[endEffectorID]->getBoneIndex(), endEffector[endEffectorID]->getIKMode(), finalPos, finalRot);
			}
			
			if (hmm->hmmRecording()) hmm->recordMovement(lastTime, endEffector[endEffectorID]->getName(), finalPos, finalRot);
			if (hmm->hmmRecognizing()) hmm->recordMovement(lastTime, endEffector[endEffectorID]->getName(), finalPos, finalRot);
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

	void updateTransAndRot() {
		Kore::vec3 hipPos = endEffector[hip]->getDesPosition();
		Kore::Quaternion hipRot = endEffector[hip]->getDesRotation();

		Kore::Quaternion offsetRotation = endEffector[hip]->getOffsetRotation();
		//vec3 offsetPosition = endEffector[hip]->getOffsetPosition();

		initRot = Kore::Quaternion(0, 0, 0, 1);
		initRot.rotate(Kore::Quaternion(vec3(1, 0, 0), -Kore::pi / 2.0));
		initRot.rotate(Kore::Quaternion(vec3(0, 0, 1), Kore::pi / 2.0));
		// TODO: rotation doesnt work yet
		//initRot.rotate(hipRot);
		//initRot.rotate(offsetRotation); // Add offset
		initRot.normalize();
		initRotInv = initRot.invert();

		initTrans = mat4::Translation(hipPos.x(), 0, hipPos.z()) * initRot.matrix().Transpose();
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
	
	bool colliding = false;
	Yoga yogaPose;
	
	
	void updateCharacterText() {
		if (storyLineTree->getLeftNode() != nullptr)
			speakWithCharacter1 = storyLineTree->getLeftNode()->speakWith();
		else
			speakWithCharacter1 = None;
		if (storyLineTree->getRightNode() != nullptr)
			speakWithCharacter2 = storyLineTree->getRightNode()->speakWith();
		else
			speakWithCharacter2 = None;
	}
	
	void getRandomPose() {
		// Get random pose
		pose0 = Unknown;
		pose1 = Unknown;
		if (speakWithCharacter1 != None && speakWithCharacter2 != None)
			movement->getRandomMovement(pose0, pose1);
		else if (speakWithCharacter1 != None && speakWithCharacter2 == None)
			movement->getRandomMovement(pose0);
	}
	void getNextStoryElement(bool left) {
		if (left && storyLineTree->getLeftNode() != nullptr) {
			storyLineTree->setCurrentNode(2 * storyLineTree->getCurrentNodeID() + 1);
		} else if (!left && storyLineTree->getRightNode() != nullptr) {
			storyLineTree->setCurrentNode(2 * storyLineTree->getCurrentNodeID() + 2);
		} else {
			storyLineText = storyLineTree->getLastNode()->getData();
			speakWithCharacter1 = None;
			speakWithCharacter2 = None;
			
			currentAudio = storyLineTree->getLastNode()->getAudio();
			Audio1::stop(currentAudio);
			if(currentAudio != nullptr)
				Audio1::play(currentAudio);
			
			return;
		}
		
		storyLineText = storyLineTree->getCurrentNode()->getData();
		currentAudio = storyLineTree->getCurrentNode()->getAudio();
		Audio1::stop(currentAudio);
		if(currentAudio != nullptr)
			Audio1::play(currentAudio);
		
		updateCharacterText();
		getRandomPose();
		
		log(LogLevel::Info, storyLineText);
	}
	
	void record() {
		logRawData = !logRawData;
		
		if (logRawData && !hmm->isRecordingActive() && !hmm->isRecognitionActive()) {
			Audio1::play(startRecordingSound);
			logger->startLogger("logData");
		} else if (!logRawData && !hmm->isRecordingActive() && !hmm->isRecognitionActive()) {
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

				// Update initial matrices, so that we can recognize movements no metter where the player stands
				updateTransAndRot();
				
				// Check if avatar is colliding with a plattform
				for (int i = 0; i < 3; ++i) {
					// Check collision
					colliding = sphereColliders[i]->IntersectsWith(*avatarCollider);
					if (colliding) {
						log(LogLevel::Info, "Colliding with plattform %i", i);
						
						switch (i) {
							case 0:
								yogaPose = Yoga0;
								log(Info, "Stop recognizing the motion Yoga0");
								break;
								
							case 1:
								yogaPose = Yoga1;
								log(Info, "Stop recognizing the motion Yoga1");
								break;
								
							case 2:
								yogaPose = Yoga2;
								log(Info, "Stop recognizing the motion Yoga2");
								break;
								
							default:
								log(Info, "Stop recognizing the motion Unknown");
								break;
						}
						
					}
				}
			} else {
				bool correct = hmm->stopRecognitionAndIdentify(Yoga1);
				//bool correct = hmm->stopRecognition();
				if (correct) {
					log(Info, "The movement is correct!");
					Audio1::play(correctSound);
					
					if (pose0 == yogaPose) getNextStoryElement(true);
					else if (pose1 == yogaPose) getNextStoryElement(false);
				} else {
					log(Info, "The movement is wrong");
					Audio1::play(wrongSound);
				}
				
				colliding = false;
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
		
		const int numTrackers = 5;
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
					
					// Left or Right Leg
					std::sort(trackers.begin(), trackers.begin()+2, sortByZAxis());
					initEndEffector(leftFoot, trackers[0]->getDeviceIndex(), trackers[0]->getDesPosition(), trackers[0]->getDesRotation());
					initEndEffector(rightFoot, trackers[1]->getDeviceIndex(), trackers[1]->getDesPosition(), trackers[1]->getDesRotation());
					
					// Hip
					initEndEffector(hip, trackers[2]->getDeviceIndex(), trackers[2]->getDesPosition(), trackers[2]->getDesRotation());
					
					// Left or Right Forearm
					std::sort(trackers.begin()+3, trackers.begin()+5, sortByZAxis());
					initEndEffector(leftForeArm, trackers[3]->getDeviceIndex(), trackers[3]->getDesPosition(), trackers[3]->getDesRotation());
					initEndEffector(rightForeArm, trackers[4]->getDeviceIndex(), trackers[4]->getDesPosition(), trackers[4]->getDesRotation());
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
		//log(Info, "gamepadButton buttonNr = %i value = %f", buttonNr, value);

		// Grip button => set size and reset an avatar to a default T-Pose
		if (buttonNr == 2 && value == 1) {
			calibratedAvatar = false;
			initTransAndRot();
			avatar->resetPositionAndRotation();
			setSize();
		}
		
		// Menu button => calibrate
		if (buttonNr == 1 && value == 1) {
			assignControllerAndTracker();
			calibrate();
			calibratedAvatar = true;
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
			
			if (renderTrackerAndController && !calibratedAvatar) renderAllVRDevices();
			
			if (renderAxisForEndEffector) renderCSForEndEffector();
			
			if (renderRoom) renderLivingRoom(state.pose.vrPose.eye, state.pose.vrPose.projection);

			if (render3DText) render3Dtext(state.pose.vrPose.eye, state.pose.vrPose.projection);

			renderPlattforms(state.pose.vrPose.eye, state.pose.vrPose.projection);
			
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

		if (render3DText) render3Dtext(V, P);

		renderPlattforms(V, P);
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
			}
			
			if (!calibratedAvatar) {
				avatar->resetPositionAndRotation();
				avatar->setScale(scaleFactor);
				calibrate();
				calibratedAvatar = true;
			}
			
			for (int i = 0; i < numOfEndEffectors; ++i) {
				executeMovement(i);
			}
			
			if (!dataAvailable) {
				currentFile++;
				calibratedAvatar = false;
			}
		}
		
		// Get projection and view matrix
		mat4 P = getProjectionMatrix();
		mat4 V = getViewMatrix();
		
		renderAvatar(V, P);
		
		if (renderTrackerAndController) renderAllVRDevices();
		
		if (renderAxisForEndEffector) renderCSForEndEffector();
		
		if (renderRoom) renderLivingRoom(V, P);
		
		if (render3DText) render3Dtext(V, P);
		
		renderPlattforms(V, P);
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
			case Kore::KeyLeft:
				getNextStoryElement(true);
				break;
			case Kore::KeyRight:
				getNextStoryElement(false);
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
		cLocation = pipeline->getConstantLocation("color");
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
		//avatar = new Avatar("avatar/avatar_male.ogex", "avatar/", structure);
		avatar = new Avatar("avatar/avatar_woman.ogex", "avatar/", structure);
		const float colliderRadius = 0.2f;
		avatarCollider = new SphereCollider(vec3(0, 0, 0), colliderRadius);
		sphereMesh = new MeshObject("plattform/sphere.ogex", "plattform/", structure, avatarCollider->radius);
		
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
		
		loadLivingRoomShader();
		if (renderRoom) {
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
		
		plattforms[0] = new MeshObject("plattform/plattform0.ogex", "plattform/", structure, 0.6f);
		plattforms[1] = new MeshObject("plattform/plattform1.ogex", "plattform/", structure, 0.6f);
		plattforms[2] = new MeshObject("plattform/plattform2.ogex", "plattform/", structure, 0.6f);
		Kore::Quaternion plattformRot = Kore::Quaternion(0, 0, 0, 1);
		plattformRot.rotate(Kore::Quaternion(vec3(1, 0, 0), -Kore::pi / 2.0));
		plattformRot.rotate(Kore::Quaternion(vec3(0, 0, 1), -Kore::pi / 2.0));
		plattforms[0]->M = mat4::Translation(0, 0, 0) * plattformRot.matrix().Transpose() * mat4::Scale(0.5f, 0.5f, 0.01f);
		plattforms[1]->M = mat4::Translation(0, 0, 0.8f) * plattformRot.matrix().Transpose() * mat4::Scale(0.5f, 0.5f, 0.01f);
		plattforms[2]->M = mat4::Translation(0, 0, -0.8f) * plattformRot.matrix().Transpose() * mat4::Scale(0.5f, 0.5f, 0.01f);
		sphereColliders[0] = new SphereCollider(vec3(0, 0, 0), colliderRadius);
		sphereColliders[1] = new SphereCollider(vec3(0, 0, 0.8f), colliderRadius);
		sphereColliders[2] = new SphereCollider(vec3(0, 0, -0.8f), colliderRadius);
		
		if (render3DText) {
			textMesh[Clown1] = new MeshObject("3dtext/Clown1.ogex", "3dtext/", structure, 1);
			textMesh[Clown2] = new MeshObject("3dtext/Clown2.ogex", "3dtext/", structure, 1);
			textMesh[Clown3] = new MeshObject("3dtext/Clown3.ogex", "3dtext/", structure, 1);
			textMesh[Clown4] = new MeshObject("3dtext/Clown4.ogex", "3dtext/", structure, 1);
			textMesh[Assistent] = new MeshObject("3dtext/Assistent.ogex", "3dtext/", structure, 1);
			textMesh[AssistentMagier] = new MeshObject("3dtext/AssistentMagier.ogex", "3dtext/", structure, 1);
			textMesh[Dompteur] = new MeshObject("3dtext/Dompteur.ogex", "3dtext/", structure, 1);
			textMesh[Dude] = new MeshObject("3dtext/Dude.ogex", "3dtext/", structure, 1);
			textMesh[Magier] = new MeshObject("3dtext/Magier.ogex", "3dtext/", structure, 1);
			textMesh[Zirkusdirektor] = new MeshObject("3dtext/Zirkusdirektor.ogex", "3dtext/", structure, 1);
		}
		
		logger = new Logger();
		
		hmm = new HMM(*logger);
		
		endEffector = new EndEffector*[numOfEndEffectors];
		endEffector[head] = new EndEffector(headBoneIndex);
		endEffector[hip] = new EndEffector(hipBoneIndex);
		endEffector[leftHand] = new EndEffector(leftHandBoneIndex);
		endEffector[leftForeArm] = new EndEffector(leftForeArmBoneIndex);
		endEffector[rightHand] = new EndEffector(rightHandBoneIndex);
		endEffector[rightForeArm] = new EndEffector(rightForeArmBoneIndex);
		endEffector[leftFoot] = new EndEffector(leftFootBoneIndex);
		endEffector[rightFoot] = new EndEffector(rightFootBoneIndex);
		
#ifdef KORE_STEAMVR
		VrInterface::init(nullptr, nullptr, nullptr); // TODO: Remove
#endif
		
		storyLineTree = new BinaryTree();
		storyLineText = storyLineTree->getCurrentNode()->getData();
		if (storyLineTree->getLeftNode() != nullptr)
			speakWithCharacter1 = storyLineTree->getLeftNode()->speakWith();
		if (storyLineTree->getRightNode() != nullptr)
			speakWithCharacter2 = storyLineTree->getRightNode()->speakWith();
		movement = new Movement();
		getRandomPose();
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
	correctSound = new Sound("sound/correct.wav");
	wrongSound = new Sound("sound/wrong.wav");
	startRecognitionSound = new Sound("sound/start_recognition.wav");
	
	currentAudio = storyLineTree->getCurrentNode()->getAudio();
	//Audio1::stop(currentAudio);
	if(currentAudio != nullptr)
		Audio1::play(currentAudio);
	
	System::start();
	
	return 0;
}
