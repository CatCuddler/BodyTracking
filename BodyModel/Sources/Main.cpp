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
#include <cmath>		// std::abs

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

	const int sizeOfAvatars = 4;

	//EndEffector** endEffector;
	const int numOfEndEffectors = 8;
	EndEffector** endEffectorArr[sizeOfAvatars];
	int calibratedPuppetEndeffectors[sizeOfAvatars];
	
	Logger* logger;

    HMM* hmm;
	
	double startTime;
	double lastTime;
	
	BinaryTree* storyLineTree;
	const char* storyLineText;
	
	// Audio cues
	Sound* startRecordingSound;
	Sound* stopRecordingSound;
	//Sound* correctSound;
	//Sound* wrongSound;
	//Sound* startRecognitionSound;
	
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

	// Avatar alpha shader
	VertexStructure structure_Alpha;
	Shader* vertexShader_Alpha;
	Shader* fragmentShader_Alpha;
	PipelineState* pipeline_Alpha;

	TextureUnit tex_Alpha;
	ConstantLocation pLocation_Alpha;
	ConstantLocation vLocation_Alpha;
	ConstantLocation mLocation_Alpha;
	ConstantLocation cLocation_Alpha;

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
	
	// Null terminated array of 3d text
	MeshObject* feedbackMesh[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

    // Platform mesh objects and collider
    MeshObject* platforms[] = { nullptr, nullptr, nullptr };
   
    bool showStoryElements = false;
    bool showFeedback = false;

    SphereCollider* sphereColliders[] = { nullptr, nullptr, nullptr };

	// Avatar
	//Avatar* avatar;
	SphereCollider* avatarCollider;
	MeshObject* sphereMesh;
	// Player Avatar + Trainer
	Avatar* avatars[sizeOfAvatars] = { nullptr };
	mat4 avatarPositions[(sizeOfAvatars-1)*2];

	// Difficulty
	int const difficultyRanks = 3; // the game has x = difficultyRanks it can use
	int difficulty = 0; // difficulty Rank the game uses at the given moment
	int difficultyUpper = 6;
	int difficultyLower = 4;

	//TrainerMovement
	Logger* loggerTrainer;
	Logger* loggerTrainerMovement[difficultyRanks];
	bool moveTrainer = false;
	bool stopCycle = false;
	int const maxWaitTimer = 250;
	int waitTimer = maxWaitTimer;
	bool onTask = true; //Todo:
	//int collisonWith = 3;
	int collisionLast = -1;
	// play Speed Adjustment of Yoga Movements
	float movementStep = 0.0f;
	float movementPerStep = 0.8f; // set 1.0f max
	//colored Marker
	MeshObject* coloredTrackerBaseMesh;
	vec3 coloredTrackerColors[numOfEndEffectors];
	float trackerPrecision[numOfEndEffectors];

	float hmm_head_modelProbability = 2.0f;
	float hmm_hip_modelProbability = 2.0f;
	float hmm_leftArm_modelProbability = 2.0f;
	float hmm_rightArm_modelProbability = 2.0f;
	float hmm_leftLeg_modelProbability = 2.0f;
	float hmm_rightLeg_modelProbability = 2.0f;

	float hmm_head_modelThreshold = 1.5f;
	float hmm_hip_modelThreshold = 2.0f;
	float hmm_leftArm_modelThreshold = 3.0f;
	float hmm_rightArm_modelThreshold = 1.5f;
	float hmm_leftLeg_modelThreshold = 2.0f;
	float hmm_rightLeg_modelThreshold = 3.0f;
	
	// Virtual environment
	LivingRoom* livingRoom;
	
	vec3 color0(56.0/255.0, 56.0/255.0, 56.0/255.0);
	vec3 color1(45.0/255.0, 88.0/255.0, 103.0/255.0);
	vec3 color2(30.0/255.0, 46.0/255.0, 77.0/255.0);
	
	Character speakWithCharacter1 = None;
	Character speakWithCharacter2 = None;
	
	// Variables to mirror the room and the avatar
	vec3 mirrorOver(6.05f, 0.001f, 0.04f);
	
	mat4 initTrans;
	mat4 initTransInv;
	Kore::Quaternion initRot;
	Kore::Quaternion initRotInv;

	mat4 basicTrans;
	mat4 basicTransInv;
	Kore::Quaternion basicRot;
	Kore::Quaternion basicRotInv;

	mat4 saveTrans;
	mat4 saveTransInv;
	Kore::Quaternion saveRot;
	Kore::Quaternion saveRotInv;
	
	bool calibratedAvatar = false;
	
#ifdef KORE_STEAMVR
	bool controllerButtonsInitialized = false;
	float currentUserHeight;
	bool firstPersonMonitor = false;
#endif
	
	Movement* movement;
	Yoga pose0;
	Yoga pose1;
	Yoga yogaPose = Yoga0;
	int yogaID;
	bool colliding = false;
    double waitForAudio = 0;
	int trials = 0;

	bool recording = false;
	bool showCoordinateSystem = false;

	//char* poses[3] = { "yoga2.csv",  "yoga1.csv", "yoga3.csv" };
	char* poses[3] = { "yoga2_task.csv",  "yoga1_task.csv", "yoga3_task.csv" };
	//char* poses[3] = { "y2.csv",  "yoga1_task.csv", "y3.csv" };
	char* posesStatic[3] = { "yoga2_endpose.csv",  "yoga1_endpose.csv", "yoga3_endpose.csv" };
	
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
		if (!calibratedAvatar && showCoordinateSystem) {
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
			Kore::vec3 desPosition = endEffectorArr[0][i]->getDesPosition();
			Kore::Quaternion desRotation = endEffectorArr[0][i]->getDesRotation();
			
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
			BoneNode* bone = avatars[0]->getBoneWithIndex(endEffectorArr[0][i]->getBoneIndex());
			
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

	void renderPlatform(int platformID, vec3 color) {
		Graphics4::setMatrix(mLocation, platforms[platformID]->M);
		Graphics4::setFloat3(cLocation, color);
		platforms[platformID]->render(tex);

		// Mirror platform
		mat4 platformTransMirror = getMirrorMatrix() * platforms[platformID]->M;
		Graphics4::setMatrix(mLocation, platformTransMirror);
		platforms[platformID]->render(tex);
	}
	
	void renderPlatforms(mat4 V, mat4 P) {
        Graphics4::setPipeline(pipeline);
		
		Graphics4::setMatrix(vLocation, V);
		Graphics4::setMatrix(pLocation, P);
		
		if (pose0 == Yoga0 || pose1 == Yoga0) {
			renderPlatform(0, color0);
			//renderAvatar(V, P, avatars[1]);
		}
		
		if (pose0 == Yoga1 || pose1 == Yoga1) {
			renderPlatform(1, color1);
			//renderAvatar(V, P, avatars[2]);
		}
		
		if (pose0 == Yoga2 || pose1 == Yoga2) {
			renderPlatform(2, color2);
			//renderAvatar(V, P, avatars[3]);
		}
		
		// Reset color
		Graphics4::setFloat3(cLocation, vec3(1.0, 1.0, 1.0));
		//renderAvatar(V, P, avatars[3]);
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
	}

	bool hmm_head = true;
	bool hmm_hip = false;
	bool hmm_left_arm = true;
	bool hmm_right_arm = false;
	bool hmm_left_leg = true;
	bool hmm_right_leg = false;
	
	void renderHMMFeedback(int feedbackID, bool checkmark) {
		Kore::Quaternion textRot = Kore::Quaternion(0, 0, 0, 1);
		//textRot.rotate(Kore::Quaternion(vec3(0, 0, 0), -Kore::pi / 2.0));
		Kore::Quaternion rot90y = Kore::Quaternion(0, 0.7071f, 0, 0.7071f);
		
		Kore::mat4 M = Kore::mat4::Identity();
		const float xPos = 0.8f;
		const float yPos = 3.0f;
		const float yOffset = 0.25f;
		const float zPos = -3.5f;

		const float xPos2 = 9.0f;
		const float yPos2 = 3.5f;
		const float yOffset2 = 0.25f;
		const float zPos2 = -1.5f;

		vec3 color = vec3(1, 1, 1);	// white
		Graphics4::setFloat3(cLocation, color);
		
		M = mat4::Translation(xPos, yPos - feedbackID * yOffset, zPos) * textRot.matrix().Transpose() * mat4::Scale(0.2f, 0.2f, 0.2f);
		//M = mat4::Translation(xPos, yPos - feedbackID * yOffset, zPos) * mat4::Scale(0.2f, 0.2f, 0.2f);
		Graphics4::setMatrix(mLocation, M);
		feedbackMesh[feedbackID]->render(tex);

		M = mat4::Translation(xPos2, yPos2 - feedbackID * yOffset2, zPos2) * rot90y.matrix() * textRot.matrix().Transpose() * mat4::Scale(0.2f, 0.2f, 0.2f);
		//M = mat4::Translation(xPos, yPos - feedbackID * yOffset, zPos) * mat4::Scale(0.2f, 0.2f, 0.2f);
		Graphics4::setMatrix(mLocation, M);
		feedbackMesh[feedbackID]->render(tex);

		M = mat4::Translation(xPos + 1.0f, yPos - feedbackID * yOffset, zPos) * textRot.matrix().Transpose() * mat4::Scale(0.3f, 0.3f, 0.3f);
		//M = mat4::Translation(xPos + 1.0f, yPos - feedbackID * yOffset, zPos) * mat4::Scale(0.3f, 0.3f, 0.3f);
		Graphics4::setMatrix(mLocation, M);
		if (checkmark) {
			vec3 color = vec3(0, 1, 0);	// green
			Graphics4::setFloat3(cLocation, color);
			
			feedbackMesh[CheckMark]->render(tex);
		} else {
			vec3 color = vec3(1, 0, 0); // red
			Graphics4::setFloat3(cLocation, color);
			
			feedbackMesh[CrossMark]->render(tex);
		}


		M = mat4::Translation(xPos2, yPos2 - feedbackID * yOffset2, zPos2 + 1.0f) * rot90y.matrix() * textRot.matrix().Transpose() * mat4::Scale(0.3f, 0.3f, 0.3f);
		//M = mat4::Translation(xPos + 1.0f, yPos - feedbackID * yOffset, zPos) * mat4::Scale(0.3f, 0.3f, 0.3f);
		Graphics4::setMatrix(mLocation, M);
		if (checkmark) {
			vec3 color = vec3(0, 1, 0);	// green
			Graphics4::setFloat3(cLocation, color);

			feedbackMesh[CheckMark]->render(tex);
		}
		else {
			vec3 color = vec3(1, 0, 0); // red
			Graphics4::setFloat3(cLocation, color);

			feedbackMesh[CrossMark]->render(tex);
		}
	}

	void renderFeedbackText(mat4 V, mat4 P) {
		Graphics4::setPipeline(pipeline);
	
		Graphics4::setMatrix(vLocation, V);
		Graphics4::setMatrix(pLocation, P);
        Graphics4::setFloat3(cLocation, vec3(1, 1, 1));
		
		renderHMMFeedback(Head, hmm_head);
		renderHMMFeedback(Hip, hmm_hip);
		renderHMMFeedback(LeftArm, hmm_left_arm);
		renderHMMFeedback(RightArm, hmm_right_arm);
		renderHMMFeedback(LeftLeg, hmm_left_leg);
		renderHMMFeedback(RightLeg, hmm_right_leg);
	}
	
	void renderAvatar(mat4 V, mat4 P, Avatar* avatar, mat4 mLoc = initTrans) {
		Graphics4::setPipeline(pipeline);
		
		Graphics4::setMatrix(vLocation, V);
		Graphics4::setMatrix(pLocation, P);
		Graphics4::setMatrix(mLocation, mLoc);
		Graphics4::setFloat3(cLocation, vec3(1, 1, 1));
		avatar->animate(tex);

		// Render collider
		//Graphics4::setMatrix(mLocation, sphereMesh->M);
		//sphereMesh->render(tex);

		// Mirror the avatar
		mat4 initTransMirror = getMirrorMatrix() * mLoc;
		Graphics4::setMatrix(mLocation, initTransMirror);
		avatar->animate(tex);
	}

	void renderTransparentAvatar(mat4 V, mat4 P, Avatar* avatar, mat4 mLoc = initTrans) {
		Graphics4::setPipeline(pipeline_Alpha);

		Graphics4::setMatrix(vLocation_Alpha, V);
		Graphics4::setMatrix(pLocation_Alpha, P);
		Graphics4::setMatrix(mLocation, mLoc);
		Graphics4::setFloat3(cLocation_Alpha, vec3(1, 1, 1));
		avatar->animate(tex_Alpha);

		// Render collider
		//Graphics4::setMatrix(mLocation, sphereMesh->M);
		//sphereMesh->render(tex);

		// Mirror the avatar
		mat4 initTransMirror = getMirrorMatrix() * mLoc;
		Graphics4::setMatrix(mLocation, initTransMirror);
		avatar->animate(tex);
	}

	void renderColoredTracker(mat4 V, mat4 P, Avatar* avatar, mat4 mLoc = basicTrans) {
		for (int i = 0; i < numOfEndEffectors; ++i) {
			Graphics4::setPipeline(pipeline);
			Graphics4::setMatrix(vLocation, V);
			Graphics4::setMatrix(pLocation, P);
			Graphics4::setFloat3(cLocation, coloredTrackerColors[i]);

			BoneNode* bone = avatar->getBoneWithIndex(endEffectorArr[0][i]->getBoneIndex());

			vec3 endEffectorPos = bone->getPosition();
			endEffectorPos = basicTrans * vec4(endEffectorPos.x()*avatar->scale, endEffectorPos.y()* avatar->scale, endEffectorPos.z()* avatar->scale, 1);
			//endEffectorPos = basicTrans * vec4(endEffectorPos.x()*avatar->scale*1.33f, endEffectorPos.y()* avatar->scale*1.33f, endEffectorPos.z()* avatar->scale*1.33f, 1);
			Kore::Quaternion endEffectorRot = basicRot.rotated(bone->getOrientation());

			Kore::mat4 M = mat4::Translation(endEffectorPos.x(), endEffectorPos.y(), endEffectorPos.z()) * mLoc;
			// coloredTracker
			Graphics4::setMatrix(mLocation, M);

			//coloredTracker[i]->render(tex);
			coloredTrackerBaseMesh->render(tex);

			// Mirror the coloredTracker
			mat4 initTransMirror = getMirrorMatrix() * M;
			Graphics4::setMatrix(mLocation, initTransMirror);
			coloredTrackerBaseMesh->render(tex);

			// Reset color
			Graphics4::setFloat3(cLocation, vec3(1.0, 1.0, 1.0));
		}
	}

	void renderTransparentTrainers(mat4 V, mat4 P) {
		// pose0 and pose1 need to be checked, else collision would be checked for the inactive poses
		if (pose0 == Yoga0 || pose1 == Yoga0) {
			// Check collision
			if (sphereColliders[0]->IntersectsWith(*avatarCollider)) {
				renderColoredTracker(V, P, avatars[4]);
				renderTransparentAvatar(V, P, avatars[4]);
			}
		}

		if (pose0 == Yoga1 || pose1 == Yoga1) {
			// Check collision
			if (sphereColliders[1]->IntersectsWith(*avatarCollider)) {
				renderColoredTracker(V, P, avatars[5]);
				renderTransparentAvatar(V, P, avatars[5]);
			}
		}

		if (pose0 == Yoga2 || pose1 == Yoga2) {
			// Check collision
			if (sphereColliders[2]->IntersectsWith(*avatarCollider)) {
				renderColoredTracker(V, P, avatars[6]);
				renderTransparentAvatar(V, P, avatars[6]);
			}
		}
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

	void initBasics() {
		basicRot = initRot;
		basicRotInv = initRotInv;
		basicTrans = initTrans;
		basicTransInv = initTransInv;
	}

	void updateTransAndRot() {
		Kore::vec3 hipPos = endEffectorArr[0][hip]->getDesPosition();
		Kore::Quaternion hipRot = endEffectorArr[0][hip]->getDesRotation();

		Kore::Quaternion offsetRotation = endEffectorArr[0][hip]->getOffsetRotation();
		//vec3 offsetPosition = endEffectorArr[0][hip]->getOffsetPosition();

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

	void changeTransRot() {
		saveTrans = initTrans;
		saveTransInv = initTransInv;
		saveRot = initRot;
		saveRotInv = initRotInv;

		initTrans = basicTrans;
		initTransInv = basicTransInv;
		initRot = basicRot;
		initRotInv = basicRotInv;
	}

	void changeTransRotUndo(){
		initTrans = saveTrans;
		initTransInv = saveTransInv;
		initRot = saveRot;
		initRotInv = saveRotInv;
	}

	void executeMovement(int endEffectorID, Avatar* ava = avatars[0], bool setPose = false/*init a new yoga pose*/) {
		int endEffectorUsed = ava->getAvatarID();
		if (endEffectorUsed > 0) changeTransRot();
		Kore::vec3 desPosition = endEffectorArr[endEffectorUsed][endEffectorID]->getDesPosition();
		Kore::Quaternion desRotation = endEffectorArr[endEffectorUsed][endEffectorID]->getDesRotation();
		// Save raw data when on the player avatar
		if (logRawData && (endEffectorUsed == 0)) logger->saveData(endEffectorArr[endEffectorUsed][endEffectorID]->getName(), desPosition, desRotation, ava->scale);

		if (calibratedAvatar || (endEffectorUsed != 0)) {
			// Transform desired position/rotation to the character local coordinate system
			desPosition = initTransInv * vec4(desPosition.x(), desPosition.y(), desPosition.z(), 1);
			desRotation = initRotInv.rotated(desRotation);

			// Add offset
			Kore::Quaternion offsetRotation = endEffectorArr[endEffectorUsed][endEffectorID]->getOffsetRotation();
			vec3 offsetPosition = endEffectorArr[endEffectorUsed][endEffectorID]->getOffsetPosition();
			Kore::Quaternion finalRot = desRotation.rotated(offsetRotation);
			vec3 finalPos = mat4::Translation(desPosition.x(), desPosition.y(), desPosition.z()) * finalRot.matrix().Transpose() * mat4::Translation(offsetPosition.x(), offsetPosition.y(), offsetPosition.z()) * vec4(0, 0, 0, 1);

			if (endEffectorID == hip) {
				ava->setFixedPositionAndOrientation(endEffectorArr[endEffectorUsed][endEffectorID]->getBoneIndex(), finalPos, finalRot);

				if (endEffectorUsed == 0) {
					// Update the sphere collider for the player avatar
					avatarCollider->center = vec3(endEffectorArr[endEffectorUsed][hip]->getDesPosition().x(), 0, endEffectorArr[endEffectorUsed][hip]->getDesPosition().z());
					sphereMesh->M = mat4::Translation(avatarCollider->center.x(), avatarCollider->center.y(), avatarCollider->center.z());
				}

			}
			else if (endEffectorID == head || endEffectorID == leftForeArm || endEffectorID == rightForeArm || endEffectorID == leftFoot || endEffectorID == rightFoot) {
				ava->setDesiredPositionAndOrientation(endEffectorArr[endEffectorUsed][endEffectorID]->getBoneIndex(), endEffectorArr[endEffectorUsed][endEffectorID]->getIKMode(), finalPos, finalRot);
			}
			else if (endEffectorID == leftHand || endEffectorID == rightHand) {
				if (!setPose) ava->setFixedOrientation(endEffectorArr[endEffectorUsed][endEffectorID]->getBoneIndex(), finalRot);
				// using setDesiredPositionAndOrientation will fix the hand positioning problems on loading a yoga positon
				else ava->setDesiredPositionAndOrientation(endEffectorArr[endEffectorUsed][endEffectorID]->getBoneIndex(), endEffectorArr[endEffectorUsed][endEffectorID]->getIKMode(), finalPos, finalRot);
			}
			if (endEffectorUsed == 0) { // only for the player Avatar
				//log(Kore::Info, "endEffectorUsed %i", endEffectorUsed);
				if (recording) hmm->recordMovement(lastTime, endEffectorArr[0][endEffectorID]->getName(), finalPos, finalRot);
			}
		}
		if (endEffectorUsed > 0) changeTransRotUndo();
	}

	void runCalibrate(int endEffectorID, Avatar* ava = avatars[0]) {
		int endEffectorUsed = ava->getAvatarID();

		mat4 usedTransInv = initTransInv;
		Kore::Quaternion usedRotInv = initRotInv;
		if (endEffectorUsed > 0) {
			mat4 usedTransInv = basicTransInv;
			Kore::Quaternion usedRotInv = basicRotInv;
		}

		Kore::vec3 desPosition = endEffectorArr[endEffectorUsed][endEffectorID]->getDesPosition();
		Kore::Quaternion desRotation = endEffectorArr[endEffectorUsed][endEffectorID]->getDesRotation();

		// Transform desired position/rotation to the character local coordinate system
		desPosition = usedTransInv * vec4(desPosition.x(), desPosition.y(), desPosition.z(), 1);
		//desPosition = initTransInv * vec4(desPosition.x(), desPosition.y(), desPosition.z(), 1);
		desRotation = usedRotInv.rotated(desRotation);
		//desRotation = initRotInv.rotated(desRotation);

		// Get actual position/rotation of the character skeleton
		BoneNode* bone = ava->getBoneWithIndex(endEffectorArr[endEffectorUsed][endEffectorID]->getBoneIndex());
		vec3 targetPos = bone->getPosition();
		Kore::Quaternion targetRot = bone->getOrientation();

		endEffectorArr[endEffectorUsed][endEffectorID]->setOffsetPosition((mat4::Translation(desPosition.x(), desPosition.y(), desPosition.z()) * targetRot.matrix().Transpose()).Invert() * mat4::Translation(targetPos.x(), targetPos.y(), targetPos.z()) * vec4(0, 0, 0, 1));
		endEffectorArr[endEffectorUsed][endEffectorID]->setOffsetRotation((desRotation.invert()).rotated(targetRot));
	}

	void calibrate() {
		initTransAndRot();		// TODO: remove or necessary while in VR Mode?	
		for (int endEffectorID = 0; endEffectorID < numOfEndEffectors; ++endEffectorID) {
			runCalibrate(endEffectorID);
		}
	}
	
	void setPose(Avatar* avatar, char* fileName, bool calibrate = false) {
		int endEffectorUsed = avatar->getAvatarID();
		//if (endEffectorUsed > 0) changeTransRot();
		float scaleFactor;
		Kore::vec3 desPosition[numOfEndEffectors];
		Kore::Quaternion desRotation[numOfEndEffectors];

		Logger* loggerP = new Logger();
		/*	// if we want to allow loading the end position of any csv animation record file:
		bool data = true;
		while (data) {
			data = loggerP->readData(numOfEndEffectors, fileName, desPosition, desRotation, scaleFactor);
		*/
		loggerP->readData(numOfEndEffectors, fileName, desPosition, desRotation, scaleFactor);
		if (calibrate) {
			avatar->setScale(scaleFactor);
		}
		for (int endEffectorID = 0; endEffectorID < numOfEndEffectors; endEffectorID++) {
			endEffectorArr[endEffectorUsed][endEffectorID]->setDesPosition(vec3(desPosition[endEffectorID].x(), desPosition[endEffectorID].y(), desPosition[endEffectorID].z()));
			endEffectorArr[endEffectorUsed][endEffectorID]->setDesRotation(desRotation[endEffectorID]);
			if (calibrate) {
				runCalibrate(endEffectorID, avatar);
			}
			else {
				executeMovement(endEffectorID, avatar, true);
			}
		}
		//if (endEffectorUsed > 0) changeTransRotUndo();
	}

	void calibratePuppets(char* fileName = "calibratePuppet.csv") {
		for (int i = 1; i < sizeOfAvatars; i++) {
			avatars[i]->resetPositionAndRotation();
			setPose(avatars[i], fileName, true);
		}
	}
	
	void calculateColor(int i, float modelProbability, float modelThreshold) {
		double r = 0.0;
		double g = 0.0;

		float colorRange = modelThreshold / 2.0f;
		r = (modelProbability - (modelThreshold - colorRange)) / colorRange * 247.0;
		g = (colorRange - (modelProbability - modelThreshold)) / colorRange * 247.0;

		coloredTrackerColors[i] = vec3(r / 255.0, g / 255.0, 0.0 / 255.0);
	}

	void updateColors() {
		hmm->getFeedbackModel(hmm_head_modelProbability, hmm_hip_modelProbability, hmm_leftArm_modelProbability, hmm_rightArm_modelProbability, hmm_leftLeg_modelProbability, hmm_rightLeg_modelProbability,
			hmm_head_modelThreshold, hmm_hip_modelThreshold, hmm_leftArm_modelThreshold, hmm_rightArm_modelThreshold, hmm_leftLeg_modelThreshold, hmm_rightLeg_modelThreshold);

		calculateColor(0, hmm_head_modelProbability, hmm_head_modelThreshold);
		calculateColor(1, hmm_hip_modelProbability, hmm_hip_modelThreshold);
		calculateColor(2, hmm_leftArm_modelProbability, hmm_leftArm_modelThreshold);
		calculateColor(3, hmm_leftArm_modelProbability, hmm_leftArm_modelThreshold);
		calculateColor(4, hmm_rightArm_modelProbability, hmm_rightArm_modelThreshold);
		calculateColor(5, hmm_rightArm_modelProbability, hmm_rightArm_modelThreshold);
		calculateColor(6, hmm_leftLeg_modelProbability, hmm_leftLeg_modelThreshold);
		calculateColor(7, hmm_rightLeg_modelProbability, hmm_rightLeg_modelThreshold);
		}

	void trainerMovement(Avatar* avatar, Logger* loggerUsed, char* fileName) {
		if (moveTrainer){
			float scaleFactor;
			Kore::vec3 desPosition[numOfEndEffectors];
			Kore::Quaternion desRotation[numOfEndEffectors];
			bool dataAvailable;
			// Speed up the playback of Yogamovement
			while (movementStep < 1.0f) {
				dataAvailable = loggerUsed->readData(numOfEndEffectors, fileName, desPosition, desRotation, scaleFactor);
				if (dataAvailable) movementStep += movementPerStep;
				else movementStep = 1.0f;
			}
			movementStep--;

			if (dataAvailable) {
				for (int i = 0; i < numOfEndEffectors; ++i) {
					endEffectorArr[avatar->getAvatarID()][i]->setDesPosition(desPosition[i]);
					endEffectorArr[avatar->getAvatarID()][i]->setDesRotation(desRotation[i]);
				}
				for (int i = 0; i < numOfEndEffectors; ++i) {
					executeMovement(i, avatar, true);
				}
			}
			else { moveTrainer = false; }
		}
	}
	
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
			movement->getRandomMovement(pose0, pose1, yogaPose);
		else if (speakWithCharacter1 != None && speakWithCharacter2 == None)
			movement->getRandomMovement(pose0, yogaPose);
	}
	
	bool intro = false;
	void initGame() {
		if (!intro) {
			storyLineTree = new BinaryTree();
			storyLineText = storyLineTree->getCurrentNode()->getData();
			if (storyLineTree->getLeftNode() != nullptr)
				speakWithCharacter1 = storyLineTree->getLeftNode()->speakWith();
			if (storyLineTree->getRightNode() != nullptr)
				speakWithCharacter2 = storyLineTree->getRightNode()->speakWith();
			movement = new Movement();
			getRandomPose();
			
			// Play intro only once
			currentAudio = storyLineTree->getCurrentNode()->getAudio();
			if(currentAudio != nullptr)
				Audio1::play(currentAudio);
				
			intro = true;
		}
	}
	
	bool outro = false;
	void finishGame() {
		if (!outro) {
			storyLineText = storyLineTree->getLastNode()->getData();
			speakWithCharacter1 = None;
			speakWithCharacter2 = None;
			
			// Play outro only once
			Audio1::stop(currentAudio);
			currentAudio = storyLineTree->getLastNode()->getAudio();
			if(currentAudio != nullptr)
				Audio1::play(currentAudio);
			
			outro = true;
		}
	}

    void getNextStoryElement(bool left) {
        if (left && storyLineTree->getLeftNode() != nullptr) {
            storyLineTree->setCurrentNode(storyLineTree->getLeftNode()->getID());
        } else if (!left && storyLineTree->getRightNode() != nullptr) {
            storyLineTree->setCurrentNode(storyLineTree->getRightNode()->getID());
        }
    
        Audio1::stop(currentAudio);
        storyLineText = storyLineTree->getCurrentNode()->getData();
        currentAudio = storyLineTree->getCurrentNode()->getAudio();
        if(currentAudio != nullptr) {
            Audio1::play(currentAudio);
            waitForAudio = 0;
        }
    
        updateCharacterText();
        getRandomPose();
    
        log(LogLevel::Info, storyLineText);
    }

	void loadTrainer(int i) {
		loggerTrainerMovement[i] = new Logger();

		setPose(avatars[i + 1], posesStatic[i]);
		//if (difficulty == 0) avatars[i + 1]->setScale(avatars[0]->scale * 0.75f);
		//else avatars[i + 1]->setScale(avatars[0]->scale);
		avatars[i + 1]->setScale(avatars[0]->scale);
	}

	void difficultySet() {
		//for (int i = 0; i < difficultyRanks; i++) { loggerTrainerMovement[i] = new Logger(); }
		for (int i = 1; i < sizeOfAvatars; i++) {
			loadTrainer(i - 1);
		}

		waitTimer = maxWaitTimer;
		moveTrainer = false;
		stopCycle = false;
	}

	void collisionCheck() {
		int collisionSave = collisionLast;
		//check collison for all plattforms
		for (int i = 0; i < 3; ++i) {
			// Check collision for one plattform vs the player avatar
			if (sphereColliders[i]->IntersectsWith(*avatarCollider)) {
				if (i == 0 && (pose0 == Yoga0 || pose1 == Yoga0)) {
					collisionLast = i;
				}
				else if (i == 1 && (pose0 == Yoga1 || pose1 == Yoga1)) {
					collisionLast = i;
				}
				else if (i == 2 && (pose0 == Yoga2 || pose1 == Yoga2)) {
					collisionLast = i;
				}
			}
			/*
			if (sphereColliders[i]->IntersectsWith(*avatarCollider)) {
				if (i == 1 && (pose0 == Yoga1 || pose1 == Yoga1)) {
					collisionLast = i;
				}
			}
			if (sphereColliders[i]->IntersectsWith(*avatarCollider)) {
				if (i == 2 && (pose0 == Yoga2 || pose1 == Yoga2)) {
					collisionLast = i;
				}
			}
			*/
		}
		if (collisionSave != collisionLast) {
			difficultySet();
			trials = 0;
		}
	}

	void neutralColoredTracker() {
		for (int i = 0; i < numOfEndEffectors; i++) {
			coloredTrackerColors[i] = vec3(0.0 / 255.0, 0.0 / 255.0, 247.0 / 255.0);			// blue
		}
	}

	void difficultyIncrease() {
		if (difficulty < (difficultyRanks - 1)) {
			difficulty++;
			difficultySet();
		}
	}

	void difficultyDecrease() {
		if (difficulty > 0) {
			difficulty--;
			difficultySet();
		}
	}

	void startTrainer(int i) {
		loadTrainer(i);

		waitTimer = 0;
		moveTrainer = true;
	}

	void record() {
		/*logRawData = !logRawData;
		
		if (logRawData) {
			Audio1::play(startRecordingSound);
			logger->startLogger("logData");
		} else {
			Audio1::play(stopRecordingSound);
			logger->endLogger();
		}*/
		
		// HMM
		if(hmm->isRecordingActive()) {
			// Recording a movement
			if (recording) {
				Audio1::play(startRecordingSound);
				hmm->startRecording(endEffectorArr[0][head]->getDesPosition(), endEffectorArr[0][head]->getDesRotation());
			} else {
				Audio1::play(stopRecordingSound);
				hmm->stopRecording();
			}
		} else if(hmm->isRecognitionActive()) {
			// Recognizing a movement
			if (recording) {
				showFeedback = false;
				
				//Audio1::play(startRecognitionSound);
				log(Info, "Start recognizing the motion");
				hmm->startRecognition(endEffectorArr[0][head]->getDesPosition(), endEffectorArr[0][head]->getDesRotation());

				// Update initial matrices, so that we can recognize movements no metter where the player stands
				updateTransAndRot();
				
				// Check if avatar is colliding with a platform
				for (int i = 0; i < 3; ++i) {
					// Check collision
					colliding = sphereColliders[i]->IntersectsWith(*avatarCollider);
					if (colliding) {
						log(LogLevel::Info, "Colliding with platform %i", i);
						//set the Trainer into motion for the yoga Pose selected by the player
						startTrainer(collisionLast);
						if (difficulty == 0) stopCycle = true;
						
						switch (i) {
							case 0:
								yogaPose = Yoga0;
								yogaID = 0;
								log(Info, "Stop recognizing the motion Yoga0");
								break;
								
							case 1:
								yogaPose = Yoga1;
								yogaID = 1;
								log(Info, "Stop recognizing the motion Yoga1");
								break;
								
							case 2:
								yogaPose = Yoga2;
								yogaID = 2;
								log(Info, "Stop recognizing the motion Yoga2");
								break;
								
							default:
								yogaID = -1;
								log(Info, "Stop recognizing the motion Unknown");
								break;
						}
					}
				}
			} else {
				bool correct = hmm->stopRecognitionAndIdentify(yogaPose);
				++trials;

				hmm->getFeedback(hmm_head, hmm_hip, hmm_left_arm, hmm_right_arm, hmm_left_leg, hmm_right_leg);
				logger->saveEvaluationData(lastTime, storyLineTree->getCurrentNode()->getID(), yogaID, difficulty, trials, hmm_head, hmm_hip, hmm_left_arm, hmm_right_arm, hmm_left_leg, hmm_right_leg);
				
				//bool correct = hmm->stopRecognition();
				if (correct || trials > 10) {
					log(Info, "The movement is correct!");
					if (trials > 10) log(Info, "more than10 Trials");
					//Audio1::play(correctSound);
					
					showFeedback = false;

					//adjust difficulty if necessary
					if (trials > difficultyUpper) difficultyDecrease();
					else if (trials < difficultyLower) difficultyIncrease();

					neutralColoredTracker();
					collisionLast = -1;
					stopCycle = false;

					if (pose0 == yogaPose) getNextStoryElement(true);
					else if (pose1 == yogaPose) getNextStoryElement(false);

					trials = 0;
				} else {
					log(Info, "The movement is wrong (Trial %i)", trials);
					updateColors();
					//Audio1::play(wrongSound);

					showFeedback = true;
				}
				
				colliding = false;
			}
		}
	}

#ifdef KORE_STEAMVR
	void setSize() {
		float currentAvatarHeight = avatars[0]->getHeight();
		
		SensorState state = VrInterface::getSensorState(0);
		vec3 hmdPos = state.pose.vrPose.position; // z -> face, y -> up down
		currentUserHeight = hmdPos.y();
		
		float scale = currentUserHeight / currentAvatarHeight;
		//avatar->setScale(scale);
		for (int i = 0; i < sizeOfAvatars; i++) {
			avatars[i]->setScale(scale);
			//if (i < 4) avatars[i]->setScale(scale);
			//else avatars[i]->setScale(scale*0.75f);
		}
		log(Info, "current avatar height %f, current user height %f ==> scale %f", currentAvatarHeight, currentUserHeight, scale);
	}
	
	void initEndEffector(int efID, int deviceID, vec3 pos, Kore::Quaternion rot) {
		endEffectorArr[0][efID]->setDeviceIndex(deviceID);
		endEffectorArr[0][efID]->setDesPosition(pos);
		endEffectorArr[0][efID]->setDesRotation(rot);
		
		log(Info, "%s, device id: %i", endEffectorArr[0][efID]->getName(), deviceID);
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
		/* deactivated for Eval
		// Grip button => set size and reset an avatar to a default T-Pose
		if (buttonNr == 2 && value == 1) {
			calibratedAvatar = false;
			initTransAndRot();
			avatars[0]->resetPositionAndRotation();
			setSize();
		}
		
		// Menu button => calibrate
		if (buttonNr == 1 && value == 1) {
			assignControllerAndTracker();
			calibrate();
			collisionLast = -1;
			moveTrainer = false;
			stopCycle = false;
			calibratedAvatar = true;
			log(Info, "Calibrate avatar");
		}

		// Trackpad => start game
		if (buttonNr == 32 && value == 1) {
			if (calibratedAvatar) initGame();
		}
		*/
		// Trigger button => record data
		if (buttonNr == 33 && value == 1) {
			// Trigger button pressed
			log(Info, "Trigger button pressed");
			if (calibratedAvatar && showStoryElements) {
				recording = true;
				record();
			}
		}

		if (buttonNr == 33 && value == 0) {
			// Trigger button released
			log(Info, "Trigger button released");
			if (calibratedAvatar && showStoryElements) {
				recording = false;
				record();
			}
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

		//assert(count == 2);
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
		Graphics4::setPipeline(pipeline);				// TODO: not needed, renderavatar does this allready later, bevor its used the first time. - VR-Mode?
		
#ifdef KORE_STEAMVR
		VrInterface::begin();

		if (!controllerButtonsInitialized) initButtons();
		
		VrPoseState vrDevice;
		for (int i = 0; i < numOfEndEffectors; ++i) {
			if (endEffectorArr[0][i]->getDeviceIndex() != -1) {

				if (i == head) {
					SensorState state = VrInterface::getSensorState(0);

					// Get HMD position and rotation
					endEffectorArr[0][i]->setDesPosition(state.pose.vrPose.position);
					endEffectorArr[0][i]->setDesRotation(state.pose.vrPose.orientation);
				} else {
					vrDevice = VrInterface::getController(endEffectorArr[0][i]->getDeviceIndex());

					// Get VR device position and rotation
					endEffectorArr[0][i]->setDesPosition(vrDevice.vrPose.position);
					endEffectorArr[0][i]->setDesRotation(vrDevice.vrPose.orientation);
				}

				executeMovement(i);
			}
		}


		if (difficulty < 2 && moveTrainer == false && !stopCycle) {
			// Wait for maxWaitTimer Frames in End Position
			if (waitTimer > 0) waitTimer--;
			else {
				waitTimer = maxWaitTimer;
				moveTrainer = true;
			}
		}
		//check for collision Player - Plattforms
		collisionCheck();
		
		// Render for both eyes
		SensorState state;
		for (int j = 0; j < 2; ++j) {
			VrInterface::beginRender(j);
			
			Graphics4::clear(Graphics4::ClearColorFlag | Graphics4::ClearDepthFlag, Graphics1::Color::Black, 1.0f, 0);
			
			state = VrInterface::getSensorState(j);
			
			renderAvatar(state.pose.vrPose.eye, state.pose.vrPose.projection, avatars[0]);
			
			if (renderTrackerAndController && !calibratedAvatar) renderAllVRDevices();
			
			if (renderAxisForEndEffector) renderCSForEndEffector();
			
			if (renderRoom) renderLivingRoom(state.pose.vrPose.eye, state.pose.vrPose.projection);

			if (showStoryElements) render3Dtext(state.pose.vrPose.eye, state.pose.vrPose.projection);

			if (showFeedback) renderFeedbackText(state.pose.vrPose.eye, state.pose.vrPose.projection);

			if (showStoryElements) renderPlatforms(state.pose.vrPose.eye, state.pose.vrPose.projection);

			if (showStoryElements && calibratedAvatar && collisionLast >= 0 && collisionLast < 3) {
				switch (difficulty) {
					case 2:
						renderAvatar(state.pose.vrPose.eye, state.pose.vrPose.projection, avatars[collisionLast + 1], avatarPositions[collisionLast]);
						break;
					case 1:
						renderAvatar(state.pose.vrPose.eye, state.pose.vrPose.projection, avatars[collisionLast + 1], avatarPositions[collisionLast]);
						break;
					case 0:
						renderColoredTracker(state.pose.vrPose.eye, state.pose.vrPose.projection, avatars[collisionLast + 1], avatarPositions[collisionLast+3]);
						renderTransparentAvatar(state.pose.vrPose.eye, state.pose.vrPose.projection, avatars[collisionLast + 1], avatarPositions[collisionLast+3]);
						break;
					default:
						break;
				}
			}

			VrInterface::endRender(j);
		}
		
		VrInterface::warpSwap();
		
		Graphics4::restoreRenderTarget();
		Graphics4::clear(Graphics4::ClearColorFlag | Graphics4::ClearDepthFlag, Graphics1::Color::Black, 1.0f, 0);
		
		// Render on monitor
		mat4 P = getProjectionMatrix();
		mat4 V = getViewMatrix();

		if (firstPersonMonitor) renderAvatar(state.pose.vrPose.eye, state.pose.vrPose.projection, avatars[0]);
		else renderAvatar(V, P, avatars[0]);
		
		if (renderTrackerAndController && !calibratedAvatar) renderAllVRDevices();
		
		if (renderAxisForEndEffector) renderCSForEndEffector();
		
		if (renderRoom) {
			if (firstPersonMonitor) renderLivingRoom(state.pose.vrPose.eye, state.pose.vrPose.projection);
			else renderLivingRoom(V, P);
		}

		if (showStoryElements) {
			if (firstPersonMonitor) render3Dtext(state.pose.vrPose.eye, state.pose.vrPose.projection);
			else render3Dtext(V, P);
		}

		if (showFeedback) {
			if (firstPersonMonitor) renderFeedbackText(state.pose.vrPose.eye, state.pose.vrPose.projection);
			else renderFeedbackText(V, P);
		}

		if (showStoryElements) {
			if (firstPersonMonitor) renderPlatforms(state.pose.vrPose.eye, state.pose.vrPose.projection);
			else renderPlatforms(V, P);
		}

		mat4 P2 = P;
		mat4 V2 = V;
		
		if (!firstPersonMonitor) {
			mat4 P2 = state.pose.vrPose.projection;
			mat4 V2 = state.pose.vrPose.eye;
		}

		if (showStoryElements && calibratedAvatar && collisionLast >= 0 && collisionLast < 3) {
			switch (difficulty) {
			case 2:
				renderAvatar(V2, P2, avatars[collisionLast + 1], avatarPositions[collisionLast]);
				break;
			case 1:
				trainerMovement(avatars[collisionLast + 1], loggerTrainerMovement[collisionLast], poses[collisionLast]);
				renderAvatar(V2, P2, avatars[collisionLast + 1], avatarPositions[collisionLast]);
				break;
			case 0:
				renderColoredTracker(V2, P2, avatars[collisionLast + 1], avatarPositions[collisionLast+3]);
				trainerMovement(avatars[collisionLast + 1], loggerTrainerMovement[collisionLast], poses[collisionLast]);
				renderTransparentAvatar(V2, P2, avatars[collisionLast + 1], avatarPositions[collisionLast+3]);
				break;
			default:
				break;
			}
		}
#else
		// Read line
		float scaleFactor;
		Kore::vec3 desPosition[numOfEndEffectors];
		Kore::Quaternion desRotation[numOfEndEffectors];
		if (currentFile < numFiles) {
			bool dataAvailable = logger->readData(numOfEndEffectors, files[currentFile], desPosition, desRotation, scaleFactor);
			
			for (int i = 0; i < numOfEndEffectors; ++i) {
				endEffectorArr[0][i]->setDesPosition(desPosition[i]);
				endEffectorArr[0][i]->setDesRotation(desRotation[i]);
			}	
			
			if (!calibratedAvatar) {
				avatars[0]->resetPositionAndRotation();
				avatars[0]->setScale(scaleFactor);
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
		
		// render player avatar
		renderAvatar(V, P, avatars[0]);
		
		renderPlatform(0, color0);
		renderPlatform(1, color1);
		renderPlatform(2, color2);
		if (renderTrackerAndController && !calibratedAvatar) renderAllVRDevices();
		
		if (renderAxisForEndEffector) renderCSForEndEffector();
		
		if (renderRoom) renderLivingRoom(V, P);
		
		if (showStoryElements) render3Dtext(V, P);
		
		if (showFeedback) renderFeedbackText(V, P);
		renderFeedbackText(V, P);
		
        if (showStoryElements) renderPlatforms(V, P);

		if (difficulty < 2 && !stopCycle) {
			// Wait for maxWaitTimer Frames in End Position
			if (moveTrainer == false) {
				if (waitTimer > 0) waitTimer--;
				else {
					waitTimer = maxWaitTimer;
					moveTrainer = true;
				}
			}
		}
		
		if (onTask && collisionLast >= 0 && collisionLast < 3) {
			switch (difficulty) {
				case 2:
					renderAvatar(V, P, avatars[collisionLast+1], avatarPositions[collisionLast]);
					break;
				case 1:
					trainerMovement(avatars[collisionLast + 1], loggerTrainerMovement[collisionLast], poses[collisionLast]);
					renderAvatar(V, P, avatars[collisionLast + 1], avatarPositions[collisionLast]);
					break;
				case 0:
					renderColoredTracker(V, P, avatars[collisionLast + 1], avatarPositions[collisionLast+3]);
					trainerMovement(avatars[collisionLast + 1], loggerTrainerMovement[collisionLast], poses[collisionLast]);
					renderTransparentAvatar(V, P, avatars[collisionLast + 1], avatarPositions[collisionLast+3]);
					break;
				default:
					break;
			}
		}

#endif
		if (currentAudio != nullptr) {
			if (waitForAudio < currentAudio->length) {
				waitForAudio += deltaT;
				showStoryElements = false;
			}
			else {
				showStoryElements = true;

				if (speakWithCharacter1 == None && speakWithCharacter2 == None) {
					// Last node
					finishGame();
				}
			}
		}

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
			//manually change difficulty
			case Kore::KeyH:
				difficultyIncrease();
				break;
			case Kore::KeyG:
				difficultyDecrease();
				break;
			//manually change displayed pose
			case Kore::KeyU:
				if (collisionLast < 2) collisionLast++;
				break;
			case Kore::KeyZ:
				if (collisionLast > 0)collisionLast--;
				break;
			case KeyL:
				//Kore::log(Kore::LogLevel::Info, "cameraPos: (%f, %f, %f)", cameraPos.x(), cameraPos.y(), cameraPos.z());
				//Kore::log(Kore::LogLevel::Info, "camUp: (%f, %f, %f, %f)", camUp.x(), camUp.y(), camUp.z(), camUp.w());
				//Kore::log(Kore::LogLevel::Info, "camRight: (%f, %f, %f, %f)", camRight.x(), camRight.y(), camRight.z(), camRight.w());
				//Kore::log(Kore::LogLevel::Info, "camForward: (%f, %f, %f, %f)", camForward.x(), camForward.y(), camForward.z(), camForward.w());
				
				if (calibratedAvatar) {
					recording = true;
					record();
				}
				break;
			case Kore::KeyEscape:
			case KeyQ:
				logger->endEvaluationLogger();
				System::stop();
				break;
			case Kore::KeyLeft:
				yogaPose = pose0;
				getNextStoryElement(true);
				break;
			case Kore::KeyRight:
				yogaPose = pose1;
				getNextStoryElement(false);
				break;
			case Kore::KeyReturn:
				if (calibratedAvatar) initGame();
				break;
#ifdef KORE_STEAMVR
			case Kore::KeyR:
				// Set size and reset an avatar to a default T-Pose
				calibratedAvatar = false;
				initTransAndRot();
				avatars[0]->resetPositionAndRotation();
				setSize();
				break;
			case Kore::KeyC:
				// Calibrate
				assignControllerAndTracker();
				calibrate();
				collisionLast = -1;
				moveTrainer = false;
				stopCycle = false;
				calibratedAvatar = true;
				log(Info, "Calibrate avatar");
				break;
			case Kore::KeyC:
				if (calibratedAvatar) initGame();
				break;
				/*
			case Kore::KeyR:
				// Set size and reset an avatar to a default T-Pose
				calibratedAvatar = false;
				initTransAndRot();
				avatars[0]->resetPositionAndRotation();
				setSize();
				break;
			case Kore::KeyC:
				// Calibrate
				assignControllerAndTracker();
				calibrate();
				calibratedAvatar = true;
				log(Info, "Calibrate avatar");
				break;
				*/
#endif
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
			case KeyL:
				if (calibratedAvatar) {
					recording = false;
					record();
				}
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
		//pipeline = new PipelineState();			// no need to call it twice
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

	void loadAvatarShader_Alpha() {

		FileReader vs("shader_alpha.vert");
		FileReader fs("shader_alpha.frag");
		vertexShader_Alpha = new Shader(vs.readAll(), vs.size(), VertexShader);
		fragmentShader_Alpha = new Shader(fs.readAll(), fs.size(), FragmentShader);

		// This defines the structure of your Vertex Buffer
		structure_Alpha.add("pos", Float3VertexData);
		structure_Alpha.add("tex", Float2VertexData);
		structure_Alpha.add("nor", Float3VertexData);

		pipeline_Alpha = new PipelineState();
		pipeline_Alpha->inputLayout[0] = &structure_Alpha;
		pipeline_Alpha->inputLayout[1] = nullptr;
		pipeline_Alpha->vertexShader = vertexShader_Alpha;
		pipeline_Alpha->fragmentShader = fragmentShader_Alpha;
		pipeline_Alpha->depthMode = ZCompareLess;
		pipeline_Alpha->depthWrite = true;
		pipeline_Alpha->blendSource = Graphics4::SourceAlpha;
		pipeline_Alpha->blendDestination = Graphics4::InverseSourceAlpha;
		pipeline_Alpha->alphaBlendSource = Graphics4::SourceAlpha;
		pipeline_Alpha->alphaBlendDestination = Graphics4::InverseSourceAlpha;
		pipeline_Alpha->compile();

		tex_Alpha = pipeline_Alpha->getTextureUnit("tex");
		Graphics4::setTextureAddressing(tex_Alpha, Graphics4::U, Repeat);
		Graphics4::setTextureAddressing(tex_Alpha, Graphics4::V, Repeat);

		pLocation_Alpha = pipeline_Alpha->getConstantLocation("P");
		vLocation_Alpha = pipeline_Alpha->getConstantLocation("V");
		mLocation_Alpha = pipeline_Alpha->getConstantLocation("M");
		cLocation_Alpha = pipeline_Alpha->getConstantLocation("color");
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
		loadAvatarShader_Alpha();

		char* avatarChoice;

		// Male avatars
		avatarChoice = "avatar/male_0.ogex";
		//avatarChoice = "avatar/male_1.ogex";
		//avatarChoice = "avatar/male_2.ogex";
		//avatarChoice = "avatar/male_3.ogex";

		// Female avatars
		//avatarChoice = "avatar/female_0.ogex";
		//avatarChoice = "avatar/female_1.ogex";
		//avatarChoice = "avatar/female_2.ogex";
		//avatarChoice = "avatar/female_3.ogex";
		//avatarChoice = "avatar/female_4.ogex";
		/*
		//player Avatar
		avatars[0] = new Avatar("avatar/male_3.ogex", "avatar/", structure);
		//Trainer avatars
		avatars[1] = new Avatar("avatar/male_0.ogex", "avatar/", structure);
		avatars[2] = new Avatar("avatar/male_0.ogex", "avatar/", structure);
		avatars[3] = new Avatar("avatar/male_0.ogex", "avatar/", structure);
		*/
		//player Avatar
		avatars[0] = new Avatar(avatarChoice, "avatar/", structure);
		//Trainer avatars
		avatars[1] = new Avatar(avatarChoice, "avatar/", structure);
		avatars[2] = new Avatar(avatarChoice, "avatar/", structure);
		avatars[3] = new Avatar(avatarChoice, "avatar/", structure);
		
		const float colliderRadius = 0.2f;
		avatarCollider = new SphereCollider(vec3(0, 0, 0), colliderRadius);
		sphereMesh = new MeshObject("platform/sphere.ogex", "platform/", structure, avatarCollider->radius);
		
		initTransAndRot();
		initBasics();
		
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
		
		platforms[0] = new MeshObject("platform/platform0.ogex", "platform/", structure, 0.6f);
		platforms[1] = new MeshObject("platform/platform1.ogex", "platform/", structure, 0.6f);
		platforms[2] = new MeshObject("platform/platform2.ogex", "platform/", structure, 0.6f);
		Kore::Quaternion platformRot = Kore::Quaternion(0, 0, 0, 1);
		platformRot.rotate(Kore::Quaternion(vec3(1, 0, 0), -Kore::pi / 2.0));
		platformRot.rotate(Kore::Quaternion(vec3(0, 0, 1), -Kore::pi / 2.0));
		platforms[0]->M = mat4::Translation(0, 0, 0) * platformRot.matrix().Transpose() * mat4::Scale(0.5f, 0.5f, 0.01f);
		platforms[1]->M = mat4::Translation(0, 0, 0.8f) * platformRot.matrix().Transpose() * mat4::Scale(0.5f, 0.5f, 0.01f);
		platforms[2]->M = mat4::Translation(0, 0, -0.8f) * platformRot.matrix().Transpose() * mat4::Scale(0.5f, 0.5f, 0.01f);
		sphereColliders[0] = new SphereCollider(vec3(0, 0, 0), colliderRadius);
		sphereColliders[1] = new SphereCollider(vec3(0, 0, 0.8f), colliderRadius);
		sphereColliders[2] = new SphereCollider(vec3(0, 0, -0.8f), colliderRadius);
		
		textMesh[Clown1] = new MeshObject("3dtext/Clown1.ogex", "3dtext/", structure, 1);
        textMesh[Clown2] = new MeshObject("3dtext/Clown2.ogex", "3dtext/", structure, 1);
        textMesh[Clown3] = new MeshObject("3dtext/Clown3.ogex", "3dtext/", structure, 1);
        textMesh[Clown4] = new MeshObject("3dtext/Clown4.ogex", "3dtext/", structure, 1);
        textMesh[Assistent] = new MeshObject("3dtext/Assistent.ogex", "3dtext/", structure, 1);
        textMesh[AssistentMagier] = new MeshObject("3dtext/AssistentinMagier.ogex", "3dtext/", structure, 1);
        textMesh[Dompteur] = new MeshObject("3dtext/Dompteur.ogex", "3dtext/", structure, 1);
        textMesh[Dude] = new MeshObject("3dtext/Dude.ogex", "3dtext/", structure, 1);
        textMesh[Magier] = new MeshObject("3dtext/Magier.ogex", "3dtext/", structure, 1);
        textMesh[Zirkusdirektor] = new MeshObject("3dtext/Zirkusdirektor.ogex", "3dtext/", structure, 1);
		
		feedbackMesh[CheckMark] = new MeshObject("3dtext/checkmark.ogex", "3dtext/", structure, 1);
        feedbackMesh[CrossMark] = new MeshObject("3dtext/crossmark.ogex", "3dtext/", structure, 1);
        feedbackMesh[Head] = new MeshObject("3dtext/head.ogex", "3dtext/", structure, 1);
        feedbackMesh[Hip] = new MeshObject("3dtext/hip.ogex", "3dtext/", structure, 1);
        feedbackMesh[LeftArm] = new MeshObject("3dtext/left_arm.ogex", "3dtext/", structure, 1);
        feedbackMesh[RightArm] = new MeshObject("3dtext/right_arm.ogex", "3dtext/", structure, 1);
        feedbackMesh[LeftLeg] = new MeshObject("3dtext/left_leg.ogex", "3dtext/", structure, 1);
        feedbackMesh[RightLeg] = new MeshObject("3dtext/right_leg.ogex", "3dtext/", structure, 1);
		
		logger = new Logger();
		logger->startEvaluationLogger("yogaEval");
        
        hmm = new HMM(*logger);
		
		for (int i = 0; i < sizeOfAvatars; i++) {
			endEffectorArr[i] = new EndEffector * [numOfEndEffectors];
			endEffectorArr[i][head] = new EndEffector(headBoneIndex);
			endEffectorArr[i][hip] = new EndEffector(hipBoneIndex);
			endEffectorArr[i][leftHand] = new EndEffector(leftHandBoneIndex);
			endEffectorArr[i][leftForeArm] = new EndEffector(leftForeArmBoneIndex);
			endEffectorArr[i][rightHand] = new EndEffector(rightHandBoneIndex);
			endEffectorArr[i][rightForeArm] = new EndEffector(rightForeArmBoneIndex);
			endEffectorArr[i][leftFoot] = new EndEffector(leftFootBoneIndex);
			endEffectorArr[i][rightFoot] = new EndEffector(rightFootBoneIndex);
		}		
		//endEffector = endEffectorArr[0];

		loggerTrainer = new Logger();
		for (int i = 0; i < difficultyRanks; i++) { loggerTrainerMovement[i] = new Logger(); }

		for (int i = 0; i < sizeOfAvatars - 1; i++) {
			avatarPositions[i] = mat4::Translation(0, 0, 0.0f) * initRot.matrix().Transpose();
		}

		calibratePuppets();
		difficultySet();
		avatarPositions[0] = mat4::Translation(0.5f, 0, 0.4f) * initRot.matrix().Transpose();
		avatarPositions[1] = mat4::Translation(0.5f, 0, 1.2f) * initRot.matrix().Transpose();
		avatarPositions[2] = mat4::Translation(0.5f, 0, -0.4f) * initRot.matrix().Transpose();
		avatarPositions[3] = mat4::Translation(0, 0, 0.0f) * initRot.matrix().Transpose();
		avatarPositions[4] = mat4::Translation(0, 0, 0.8f) * initRot.matrix().Transpose();
		avatarPositions[5] = mat4::Translation(0, 0, -0.8f) * initRot.matrix().Transpose();
		//setPose(avatars[1], "yoga2_endpose.csv");
		//setPose(avatars[2], "yoga1_endpose.csv");
		//setPose(avatars[3], "yoga3_endpose.csv");
		
		coloredTrackerBaseMesh = new MeshObject("3Dobjects/Sphere_green.ogex", "3Dobjects/", structure, 1);
		neutralColoredTracker();
		
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

	//startRecognitionSound = new Sound("sound/start_recognition.wav");
	//correctSound = new Sound("sound/correct.wav");
	//wrongSound = new Sound("sound/wrong.wav");
	
	System::start();
	
	return 0;
}
