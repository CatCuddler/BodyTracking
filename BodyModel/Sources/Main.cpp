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
#endif

using namespace Kore;
using namespace Kore::Graphics4;

namespace {
	const int width = 1024;
	const int height = 768;

	int track = 1; // 0 - hands, 1 - feet
	
	Logger* logger;
	bool logData = false;
	bool readData = true;
	int line = 0;
	/*const int numOfEndEffectors = 2;
	const char* initialTransFilename = "initTransAndRot_1506685997.csv";
	const char* positionDataFilename = "positionData_1506685997.csv";*/
	
	const int numOfEndEffectors = 3;
	const char* initialTransFilename = "initTransAndRot_1506695407.csv";
	const char* positionDataFilename = "positionData_1506695407.csv";
	
	double startTime;
	double lastTime;
	float fiveSec;
	
	// Avatar
	VertexStructure structure;
	Shader* vertexShader;
	Shader* fragmentShader;
	PipelineState* pipeline;
	
	TextureUnit tex;
	ConstantLocation pLocation;
	ConstantLocation vLocation;
	ConstantLocation mLocation;
	
	// Living room
	VertexStructure structure_living_room;
	Shader* vertexShader_living_room;
	Shader* fragmentShader_living_room;
	PipelineState* pipeline_living_room;
	
	TextureUnit tex_living_room;
	ConstantLocation pLocation_living_room;
	ConstantLocation vLocation_living_room;
	ConstantLocation mLocation_living_room;
	ConstantLocation diffuse_living_room;
	ConstantLocation specular_living_room;
	ConstantLocation specular_power_living_room;
	ConstantLocation lightPosLocation_living_room;
	ConstantLocation lightCount_living_room;
	
	bool rotate = false;
	bool W, A, S, D = false;
	bool Z, X = false;
	
	MeshObject* cube1;
	MeshObject* cube2;
	Avatar* avatar;
	LivingRoom* livingRoom;
	
	Quaternion cameraRotation = Quaternion(0, 0, 0, 1);
	vec3 cameraPosition = vec3(0, 0, 0);
	
#ifdef KORE_STEAMVR
	int leftTrackerIndex = -1;
	int rightTrackerIndex = -1;
	int backTrackerIndex = -1;
#endif
	
	float angle = 0;
	vec3 desPosition1 = vec3(0, 0, 0);
	vec3 desPosition2 = vec3(0, 0, 0);
	Quaternion desRotation1 = Quaternion(0, 0, 0, 1);
	Quaternion desRotation2 = Quaternion(0, 0, 0, 1);
	
	Quaternion initDesRotationLeftHand = Quaternion(0, 0, 0, 1);
	Quaternion initDesRotationRightHand = Quaternion(0, 0, 0, 1);
	Quaternion initDesRotationLeftFoot = Quaternion(0, 0, 0, 1);
	Quaternion initDesRotationRightFoot = Quaternion(0, 0, 0, 1);
	
	mat4 initTransInv = mat4::Identity();
	mat4 initTrans = mat4::Identity();
	const mat4 hmdOffset = mat4::Translation(0, 0.2f, 0);
	Quaternion initRot = Quaternion(0, 0, 0, 1);
	Quaternion initRotInv = Quaternion(0, 0, 0, 1);
	
	bool initCharacter = false;
	
	const int leftHandBoneIndex = 10;
	const int rightHandBoneIndex = 29;
	const int leftFootBoneIndex = 49;
	const int rightFootBoneIndex = 53;
	const int renderTrackerOrTargetPosition = 1;		// 0 - dont render, 1 - render desired position, 2 - render target position
	
	void renderTracker() {
		switch (renderTrackerOrTargetPosition) {
			case 0:
				// Dont render
				break;
			case 1:
			{
				// Render desired position
				cube1->M = mat4::Translation(desPosition1.x(), desPosition1.y(), desPosition1.z()) * desRotation1.matrix().Transpose();
				Graphics4::setMatrix(mLocation, cube1->M);
				cube1->render(tex);
				
				cube2->M = mat4::Translation(desPosition2.x(), desPosition2.y(), desPosition2.z()) * desRotation2.matrix().Transpose();
				Graphics4::setMatrix(mLocation, cube2->M);
				cube2->render(tex);
				break;
			}
			case 2:
			{
				// Render target position
				vec3 targetPosition = vec3(0, 0, 0);
				Quaternion targetRotation = Quaternion(0, 0, 0, 1);
				if (track == 0) {
					targetPosition = avatar->getBonePosition(leftHandBoneIndex);
					targetRotation = avatar->getBoneGlobalRotation(leftHandBoneIndex);
				} else if (track == 1) {
					targetPosition = avatar->getBonePosition(leftFootBoneIndex);
					targetRotation = avatar->getBoneGlobalRotation(leftFootBoneIndex);
				}
				cube1->M = avatar->M * mat4::Translation(targetPosition.x(), targetPosition.y(), targetPosition.z()) * targetRotation.matrix().Transpose();
				Graphics4::setMatrix(mLocation, cube1->M);
				cube1->render(tex);
				
				if (track == 0) {
					targetPosition = avatar->getBonePosition(rightHandBoneIndex);
					targetRotation = avatar->getBoneGlobalRotation(rightHandBoneIndex);
				} else if (track == 1) {
					targetPosition = avatar->getBonePosition(rightFootBoneIndex);
					targetRotation = avatar->getBoneGlobalRotation(rightFootBoneIndex);
				}
				cube2->M = avatar->M * mat4::Translation(targetPosition.x(), targetPosition.y(), targetPosition.z()) * targetRotation.matrix().Transpose();
				Graphics4::setMatrix(mLocation, cube2->M);
				cube2->render(tex);
				break;
			}
			default:
				break;
		}
	}
	
	void renderLivingRoom(mat4 V, mat4 P) {
		Graphics4::setPipeline(pipeline_living_room);
		
		//Graphics4::setInt(lightCount_living_room, lightCount);
		//Graphics4::setFloats(lightPosLocation_living_room, (float*)lightPosition, lightCount * 4);
		livingRoom->setLights(lightCount_living_room, lightPosLocation_living_room);
		Graphics4::setMatrix(vLocation_living_room, V);
		Graphics4::setMatrix(pLocation_living_room, P);
		livingRoom->render(tex_living_room, mLocation_living_room, diffuse_living_room, specular_living_room, specular_power_living_room);
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
	
	void setDesiredPosition(Kore::vec3 desPosition, int boneIndex) {
		// Transform desired position to the character coordinate system
		vec4 finalPos = initTransInv * vec4(desPosition.x(), desPosition.y(), desPosition.z(), 1);
		avatar->setDesiredPosition(boneIndex, finalPos);
	}

	// desPosition and desRotation are global
	void setDesiredPositionAndOrientation(Kore::vec3 &desPosition, Kore::Quaternion &desRotation, const int boneIndex) {
		if (logData) {
			logger->saveData(desPosition, desRotation);
		}

		float handOffsetX = 0.02f;
		float handOffsetY = 0.02f;

		float rotOffsetY = Kore::pi / 8;
		
		Kore::Quaternion desRot = desRotation;
		if (boneIndex == rightHandBoneIndex) {
			desRot.rotate(initDesRotationRightHand);
			handOffsetX = -handOffsetX;
			desRot.rotate(Kore::Quaternion(Kore::vec3(0, 1, 0), -rotOffsetY));
		} else if (boneIndex == leftHandBoneIndex) {
			desRot.rotate(initDesRotationLeftHand);
			handOffsetX = handOffsetX;
			desRot.rotate(Kore::Quaternion(Kore::vec3(0, 1, 0), rotOffsetY));
		} else if (boneIndex == rightFootBoneIndex) {
			desRot.rotate(initDesRotationRightFoot);
		} else if (boneIndex == leftFootBoneIndex) {
			desRot.rotate(initDesRotationLeftFoot);
		}
		desRotation = desRot;
		
		// Transform desired position to the hand bone
		Kore::mat4 curPos = mat4::Translation(desPosition.x(), desPosition.y(), desPosition.z()) * desRot.matrix().Transpose() * mat4::Translation(handOffsetX, handOffsetY, 0);
		Kore::vec4 desPos = curPos * vec4(0, 0, 0, 1);
		desPosition = vec3(desPos.x(), desPos.y(), desPos.z());
		
		
		// Transform desired position to the character local coordinate system
		vec4 finalPos = initTransInv * vec4(desPos.x(), desPos.y(), desPos.z(), 1);
		Kore::Quaternion finalRot = initRotInv.rotated(desRotation);
		
//		log(Info, "loc pos %f %f %f loc rot %f %f %f %f", finalPos.x(), finalPos.y(), finalPos.z(), finalRot.x, finalRot.y, finalRot.z, finalRot.w);
		
		avatar->setDesiredPositionAndOrientation(boneIndex, finalPos, finalRot);
	}

	void setBackBonePosition(Kore::vec3 &desPosition, Kore::Quaternion &desRotation, const int boneIndex) {
		if (logData) {
			logger->saveData(desPosition, desRotation);
		}

		initRot = desRotation.rotated(Quaternion(vec3(0, 1, 0), Kore::pi));
		initRot.normalize();
		initRotInv = initRot.invert();
		avatar->M = mat4::Translation(desPosition.x(), 0, desPosition.z()) * initRot.matrix().Transpose();
		initTransInv = avatar->M.Invert();
	}
	
	void update() {
		float t = (float)(System::time() - startTime);
		double deltaT = t - lastTime;
		lastTime = t;
		
		fiveSec += deltaT;
		if (fiveSec > 1) {
			fiveSec = 0;
			
			float averageIt = avatar->getAverageIKiterationNum();
			
			if (logData) logger->saveLogData("it", averageIt);
			
			//log(Info, "Average iteration %f", averageIt);
		}
		
		// Move position of camera based on WASD keys, and XZ keys for up and down
		const float moveSpeed = 0.1f;
		if (S) {
			cameraPosition.z() += moveSpeed;
		} else if (W) {
			cameraPosition.z() -= moveSpeed;
		}
		if (A) {
			cameraPosition.x() -= moveSpeed;
		} else if (D) {
			cameraPosition.x() += moveSpeed;
		}
		if (Z) {
			cameraPosition.y() += moveSpeed;
		} else if (X) {
			cameraPosition.y() -= moveSpeed;
		}
		
		// update light pos
		//lightPosition[3] = vec4(2 * Kore::sin(2 * t), 2, 2 * Kore::cos(2 * t), 1.0);
		
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
			float currentUserHeight = hmdPos.y();
			
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
			
			log(Info, "current avatar height %f, currend user height %f, scale %f", currentAvatarHeight, currentUserHeight, scale);
			
			// Get left and right tracker index
			VrPoseState controller;
			for (int i = 0; i < 16; ++i) {
				controller = VrInterface::getController(i);
				if (controller.trackedDevice == TrackedDevice::ViveTracker) {
					vec3 trackerPos = controller.vrPose.position;
					vec4 trackerTransPos = initTransInv * vec4(trackerPos.x(), trackerPos.y(), trackerPos.z(), 1);
					if (trackerTransPos.x() > 0) {
						log(Info, "leftTrackerIndex: %i -> %i", leftTrackerIndex, i);
						leftTrackerIndex = i;
					} else {
						log(Info, "rightTrackerIndex: %i -> %i", rightTrackerIndex, i);
						rightTrackerIndex = i;
					}
					//leftTrackerIndex = -1;
					//rightTrackerIndex = i;
				} else if (controller.trackedDevice == TrackedDevice::Controller)  {
					log(Info, "backTrackerIndex: %i -> %i", backTrackerIndex, i);
					backTrackerIndex = i;
				}
			}
			
			if (logData) {
				vec4 initPos = initTrans * vec4(0, 0, 0, 1);
				logger->saveInitTransAndRot(vec3(initPos.x(), initPos.y(), initPos.z()), initRot);
			}
			
			initCharacter = true;
		}
		
		VrPoseState controller;
		if (leftTrackerIndex != -1) {
			controller = VrInterface::getController(leftTrackerIndex);

			// Get controller position and rotation
			desPosition1 = controller.vrPose.position;
			desRotation1 = controller.vrPose.orientation;
			
			if (track == 0) setDesiredPositionAndOrientation(desPosition1, desRotation1, leftHandBoneIndex);
			else if (track == 1) setDesiredPosition(desPosition1, leftFootBoneIndex);// setDesiredPositionAndOrientation(desPosition1, desRotation1, leftFootBoneIndex);
		}

		if (rightTrackerIndex != -1) {
			controller = VrInterface::getController(rightTrackerIndex);

			// Get controller position and rotation
			desPosition2 = controller.vrPose.position;
			desRotation2 = controller.vrPose.orientation;
				
			if (track == 0) setDesiredPositionAndOrientation(desPosition2, desRotation2, rightHandBoneIndex);
			else if (track == 1) setDesiredPosition(desPosition2, rightFootBoneIndex);// setDesiredPositionAndOrientation(desPosition2, desRotation2, rightFootBoneIndex);
		}

		if (backTrackerIndex != -1) {
			controller = VrInterface::getController(backTrackerIndex);

			// Get controller position
			vec3 pos = controller.vrPose.position;
			// Get controller rotation
			Quaternion rot = controller.vrPose.orientation;

			setBackBonePosition(pos, rot, backTrackerIndex);
		}
		
		for (int eye = 0; eye < 2; ++eye) {
			VrInterface::beginRender(eye);

			Graphics4::setPipeline(pipeline);
			
			Graphics4::clear(Graphics4::ClearColorFlag | Graphics4::ClearDepthFlag, Graphics1::Color::Black, 1.0f, 0);
			
			state = VrInterface::getSensorState(eye);
			Graphics4::setMatrix(vLocation, state.pose.vrPose.eye);
			Graphics4::setMatrix(pLocation, state.pose.vrPose.projection);
			
			// Render avatar
			Graphics4::setMatrix(mLocation, avatar->M);
			avatar->animate(tex, deltaT);
			
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

		mat4 P = getProjectionMatrix();
		mat4 V = getViewMatrix(); 
		if (!firstPersonMonitor) {	
			Graphics4::setMatrix(vLocation, V);
			Graphics4::setMatrix(pLocation, P);
		} else {
			Graphics4::setMatrix(vLocation, state.pose.vrPose.eye);
			Graphics4::setMatrix(pLocation, state.pose.vrPose.projection);
		}
		Graphics4::setMatrix(mLocation, avatar->M);
		avatar->animate(tex, deltaT);
		
		renderTracker();
		
		if (!firstPersonMonitor) {
			renderLivingRoom(V, P);
		}
		else {
			renderLivingRoom(state.pose.vrPose.eye, state.pose.vrPose.projection);
		}
		
		//cube->drawVertices(cube->M, state.pose.vrPose.eye, state.pose.vrPose.projection, width, height);
		//avatar->drawJoints(avatar->M, state.pose.vrPose.eye, state.pose.vrPose.projection, width, height, true);
		
#else
		if (!initCharacter) {
			avatar->setScale(0.95);	// Scale test
			
			if (readData) {
				log(Info, "Read data from file %s", initialTransFilename);
				vec3 initPos = vec3(0, 0, 0);
				logger->readInitTransAndRot(initialTransFilename, &initPos, &initRot);
				initTrans = mat4::Translation(initPos.x(), initPos.y(), initPos.z());
			}
			
			initRot.normalize();
			initRotInv = initRot.invert();
			
			avatar->M = initTrans * initRot.matrix().Transpose();
			initTransInv = (initTrans * initRot.matrix().Transpose()).Invert();
			
			initCharacter = true;
			
			if (logData) {
				vec4 initPos = initTrans * vec4(0, 0, 0, 1);
				logger->saveInitTransAndRot(vec3(initPos.x(), initPos.y(), initPos.z()), initRot);
			}
			
		}
		
		if (readData) {
			Kore::vec3 rawPos[numOfEndEffectors];
			Kore::Quaternion rawRot[numOfEndEffectors];
			if (logger->readData(line, numOfEndEffectors, positionDataFilename, rawPos, rawRot)) {
				
				for (int i = 0; i < numOfEndEffectors; ++i) {
					if (i == 0) {
						desPosition1 = rawPos[i];
						desRotation1 = rawRot[i];
						
						if (track == 0) setDesiredPositionAndOrientation(desPosition1, desRotation1, leftHandBoneIndex);
						else if (track == 1) setDesiredPosition(desPosition1, leftFootBoneIndex);//setDesiredPositionAndOrientation(desPosition1, desRotation1, leftFootBoneIndex);
					} else if (i == 1) {
						desPosition2 = rawPos[i];
						desRotation2 = rawRot[i];
						
						if (track == 0) setDesiredPositionAndOrientation(desPosition2, desRotation2, rightHandBoneIndex);
						else if (track == 1) setDesiredPosition(desPosition2, rightFootBoneIndex);//setDesiredPositionAndOrientation(desPosition2, desRotation2, rightFootBoneIndex);
					} else if (i == 2) {
						vec3 pos = rawPos[i];
						Quaternion rot = rawRot[i];
						
						initRot = rot.rotated(Quaternion(vec3(0, 1, 0), Kore::pi));
						initRot.normalize();
						initRotInv = initRot.invert();
						avatar->M = mat4::Translation(pos.x(), 0, pos.z()) * initRot.matrix().Transpose();
						initTransInv = avatar->M.Invert();
						
						//setDesiredPosition(pos, 3);
					}
				}
				
				//log(Info, "pos %f %f %f rot %f %f %f %f", desPosition1.x(), desPosition1.y(), desPosition1.z(), desRotation1.x, desRotation1.y, desRotation1.z, desRotation1.w);
			}

			//log(Info, "%i", line);
			line += numOfEndEffectors;
		} else {
			angle += 0.01;
			float radius = 0.2;
			
			// Set foot position
			//desPosition2 = vec3(-0.2 + radius * Kore::cos(angle), 0.3 + radius * Kore::sin(angle), 0.2);
			//setDesiredPosition(desPosition2, 53); // Left foot 49, right foot 53
			
			// Set hand position
			radius = 0.1;
			//desPosition = vec3(0.2 + radius * Kore::cos(angle), 0.9 + radius * Kore::sin(angle), 0.2);
			//desPosition = vec3(0.2 + radius * Kore::cos(angle), 0.9, 0.2);
			
			// Set position and orientation for the left hand
			desPosition1 = vec3(0.2, 1.0, 0.4);
			desRotation1 = Quaternion(vec3(1, 0, 0), Kore::pi/2);
			desRotation1.rotate(Quaternion(vec3(0, 1, 0), -angle));
			setDesiredPositionAndOrientation(desPosition1, desRotation1, leftHandBoneIndex);
			
			// Set position and orientation for the right hand
			//desPosition2 = (-0.2, 1.0, 0.4);
			//desRotation2 = Quaternion(vec3(1, 0, 0), Kore::pi/2);
			//desRotation2.rotate(Quaternion(vec3(0, 1, 0), angle));
			//setDesiredPositionAndOrientation(desPosition2, desRotation2, rightHandBoneIndex);
			
		}
		
		// projection matrix
		mat4 P = getProjectionMatrix();
		
		// view matrix
		mat4 V = getViewMatrix();
		
		Graphics4::setMatrix(vLocation, V);
		Graphics4::setMatrix(pLocation, P);
		
		Graphics4::setMatrix(mLocation, avatar->M);
		
		avatar->animate(tex, deltaT);
		
		//avatar->drawJoints(avatar->M, V, P, width, height, true);
		
		/*Quaternion q1 = avatar->getBoneLocalRotation(leftHandBoneIndex-1);
		Quaternion q2 = avatar->getBoneLocalRotation(leftHandBoneIndex-2);
		log(Info, "low %f %f %f %f", q1.w, q1.x, q1.y, q1.z);
		log(Info, "up %f %f %f %f", q2.w, q2.x, q2.y, q2.z);*/
		
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
		
		initDesRotationLeftHand.rotate(Quaternion(vec3(0, 1, 0), -Kore::pi / 2));
		initDesRotationRightHand.rotate(Quaternion(vec3(0, 1, 0), Kore::pi / 2));
		initDesRotationLeftFoot.rotate(Quaternion(vec3(0, 1, 0), Kore::pi / 2));
		initDesRotationRightFoot.rotate(Quaternion(vec3(0, 1, 0), -Kore::pi / 2));
		initDesRotationLeftFoot.rotate(Quaternion(vec3(1, 0, 0), Kore::pi / 2));
		initDesRotationRightFoot.rotate(Quaternion(vec3(1, 0, 0), Kore::pi / 2));

		initRot.rotate(Quaternion(vec3(1, 0, 0), -Kore::pi / 2.0));
		
		cube1 = new MeshObject("cube.ogex", "", structure, 0.05);
		cube2 = new MeshObject("cube.ogex", "", structure, 0.05);
		
		loadLivingRoomShader();
		livingRoom = new LivingRoom("sherlock_living_room/sherlock_living_room.ogex", "sherlock_living_room/", structure_living_room, 1);
		Quaternion livingRoomRot = Quaternion(0, 0, 0, 1);
		livingRoomRot.rotate(Quaternion(vec3(1, 0, 0), -Kore::pi / 2.0));
		livingRoomRot.rotate(Quaternion(vec3(0, 0, 1), Kore::pi / 2.0));
		livingRoom->M = mat4::Translation(-0.7, 0, 0) * livingRoomRot.matrix().Transpose();
		
		logger = new Logger();
		
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
	
	System::start();
	
	return 0;
}
