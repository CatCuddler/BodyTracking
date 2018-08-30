#include "pch.h"

#include <Kore/IO/FileReader.h>
#include <Kore/Graphics4/PipelineState.h>
#include <Kore/Graphics1/Color.h>
#include <Kore/Input/Keyboard.h>
#include <Kore/Input/Mouse.h>
#include <Kore/System.h>

#include "EndEffector.h"
#include "Avatar.h"
#include "LivingRoom.h"
#include "Logger.h"

#ifdef KORE_STEAMVR
#include <Kore/Vr/VrInterface.h>
#include <Kore/Vr/SensorState.h>
#include <Kore/Input/Gamepad.h>
#endif

using namespace Kore;
using namespace Kore::Graphics4;

// dynamic ik-parameter
int ikMode = 0; // 0: JT, 1: JPI, 2: DLS, 3: SVD, 4: DLS with SVD, 5: SDLS
float lambda[] =    { 0.35f,	0.05f,		0.20f,		0.25f,		0.20f,		0.018f	};
float dMaxPos[] =   { 0,        0,          0,          0,          0,          0       };
float dMaxRot[] =   { 0,        0,          0,          0,          0,          0       };
float maxSteps[] =  { 100.0f,   100.0f,     100.0f,     100.0f,     100.0f,     100.0f  };

int currentFile = 0;
int evalStepsInit = evalSteps;
float evalInitValue[] = { 0.01f,    0.01f,      0.21f,      0.36f,      0.21f,      0.01f   };

namespace {
	EndEffector** endEffector;
	vec3 desPosition[numOfEndEffectors];
	Kore::Quaternion desRotation[numOfEndEffectors];
	
	Logger* logger;
	
	double startTime;
	double lastTime;
	
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
	
	mat4 initTrans;
	mat4 initTransInv;
	Kore::Quaternion initRot;
	Kore::Quaternion initRotInv;
	
	bool calibratedAvatar = false;
	
#ifdef KORE_STEAMVR
	float currentUserHeight;
	bool initedButtons = false;
	bool firstPersonMonitor = false;
#else
	int loop = 0;
#endif
	
	void renderTracker() {
		Graphics4::setPipeline(pipeline);
		
		for(int i = 0; i < numOfEndEffectors; ++i) {
			Kore::mat4 M = mat4::Translation(desPosition[i].x(), desPosition[i].y(), desPosition[i].z()) * desRotation[i].matrix().Transpose();
			Graphics4::setMatrix(mLocation, M);
			
			if (i == hip || i == rightFoot || i == leftFoot) {
				// Render a tracker for both feet and back
				viveObjects[0]->render(tex);
			} else if (i == rightHand || i == leftHand) {
				// Render a controller for both hands
				viveObjects[1]->render(tex);
			}
			
			// Render local coordinate system
			viveObjects[2]->render(tex);
		}
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
		livingRoom->render(tex_living_room, mLocation_living_room, mLocation_living_room_inverse, diffuse_living_room, specular_living_room, specular_power_living_room);
	}
	
	void renderAvatar(mat4 V, mat4 P) {
		Graphics4::setPipeline(pipeline);
		
		Graphics4::setMatrix(vLocation, V);
		Graphics4::setMatrix(pLocation, P);
		Graphics4::setMatrix(mLocation, initTrans);
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
	
	void executeMovement(int deviceID) {
#ifdef KORE_STEAMVR
		// Save raw data
		if (logData) logger->saveData(endEffector[deviceID]->getName(), desPosition[deviceID], desRotation[deviceID], avatar->scale);
#endif
		
		if (calibratedAvatar) {
#ifdef KORE_STEAMVR
			// Add offset to endeffector
			Kore::Quaternion offsetRotation = endEffector[deviceID]->getOffsetRotation();
			vec3 offserPosition = endEffector[deviceID]->getOffsetPosition();
			
			Kore::Quaternion finalRot = desRotation[deviceID].rotated(offsetRotation);
			vec3 finalPos = mat4::Translation(desPosition[deviceID].x(), desPosition[deviceID].y(), desPosition[deviceID].z()) * finalRot.matrix().Transpose() * mat4::Translation(offserPosition.x(), offserPosition.y(), offserPosition.z()) * vec4(0, 0, 0, 1);
			
#else
			Kore::Quaternion finalRot = desRotation[deviceID];
			vec3 finalPos = desPosition[deviceID];
#endif
			
			// Transform desired position to the character local coordinate system
			finalRot = initRotInv.rotated(finalRot);
			finalPos = initTransInv * vec4(finalPos.x(), finalPos.y(), finalPos.z(), 1);
			
			if (deviceID == hip) {
				avatar->setFixedPositionAndOrientation(endEffector[deviceID]->getBoneIndex(), finalPos, finalRot);
			} else {
				avatar->setDesiredPositionAndOrientation(endEffector[deviceID]->getBoneIndex(), finalPos, finalRot);
			}
		}
	}
	
	void calibrate() {
		for (int i = 0; i < numOfEndEffectors; ++i) {
#ifdef KORE_STEAMVR
			VrPoseState controller = VrInterface::getController(endEffector[i]->getTrackerIndex());
			
			desPosition[i] = controller.vrPose.position;
			desRotation[i] = controller.vrPose.orientation;
#endif
			
			vec3 sollPos, istPos = desPosition[i];
			mat4 sollRot, istRot = desRotation[i].matrix();
			BoneNode* bone = avatar->getBoneWithIndex(endEffector[i]->getBoneIndex());
			
			sollRot = initRot.rotated(bone->getOrientation()).matrix();
			sollPos = bone->getPosition();
			sollPos = initTrans * vec4(sollPos.x(), sollPos.y(), sollPos.z(), 1);
			
			endEffector[i]->setOffsetPosition((mat4::Translation(istPos.x(), istPos.y(), istPos.z()) * sollRot.Transpose()).Invert() * mat4::Translation(sollPos.x(), sollPos.y(), sollPos.z()) * vec4(0, 0, 0, 1));
			endEffector[i]->setOffsetRotation(Kore::RotationUtility::matrixToQuaternion(sollRot * istRot.Transpose()));
		}
	}
	
	// Test this
	/*vec3 sollPos, istPos = desPosition[i];
	Kore::Quaternion sollRot, istRot = desRotation[i];
	BoneNode* bone = avatar->getBoneWithIndex(endEffector[i]->getBoneIndex());
		
	sollRot = initRot.rotated(bone->getOrientation());
	sollPos = bone->getPosition();
	sollPos = initTrans * vec4(sollPos.x(), sollPos.y(), sollPos.z(), 1);
		
	endEffector[i]->setOffsetPosition((mat4::Translation(istPos.x(), istPos.y(), istPos.z()) * sollRot.matrix().Transpose()).Invert() * mat4::Translation(sollPos.x(), sollPos.y(), sollPos.z()) * vec4(0, 0, 0, 1));
	endEffector[i]->setOffsetRotation(sollRot.rotated(istRot));
	}*/

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
	
	void initController() {
		VrPoseState controller;
		
		// Set avatar size
		setSize();
		
		// Get indices for VR devices
		for (int i = 0; i < 16; ++i) {
			controller = VrInterface::getController(i);
			
			vec3 trackerPos = controller.vrPose.position;
			vec4 trackerTransPos = initTransInv * vec4(trackerPos.x(), trackerPos.y(), trackerPos.z(), 1);
			
			if (controller.trackedDevice == TrackedDevice::ViveTracker) {
				if (trackerPos.y() < currentUserHeight / 3) {
					// Foot tracker
					if (trackerTransPos.x() > 0) {
						endEffector[rightFoot]->setTrackerIndex(i);
						log(Info, "rightFoot: %i -> %i", endEffector[rightHand]->getTrackerIndex(), i);
					} else {
						endEffector[rightHand]->setTrackerIndex(i);
						log(Info, "rightFoot: %i -> %i", endEffector[rightFoot]->getTrackerIndex(), i);
					}
				} else {
					// Hip tracker
					endEffector[hip]->setTrackerIndex(i);
					log(Info, "hip: %i -> %i", endEffector[hip]->getTrackerIndex(), i);
				}
			} else if (controller.trackedDevice == TrackedDevice::Controller) {
				// Hand controller
				if (trackerTransPos.x() > 0) {
					endEffector[leftHand]->setTrackerIndex(i);
					log(Info, "leftHand: %i -> %i", endEffector[leftHand]->getTrackerIndex(), i);
				} else {
					endEffector[rightHand]->setTrackerIndex(i);
					log(Info, "rightHand: %i -> %i", endEffector[rightHand]->getTrackerIndex(), i);
				}
			}
		}
	}
	
	void gamepadButton(int buttonNr, float value) {
		// Menu button => calibrating
		if (buttonNr == 1 && value == 1) {
			if (!currentUserHeight)
				initController();
			
			if (!calibratedAvatar) {
				calibrate();
				log(Info, "Calibrate avatar");
				calibratedAvatar = true;
			} else {
				log(Info, "Release avatar");
				calibratedAvatar = false;
			}
		}
		
		// Trigger button => logging
		if (buttonNr == 33 && value == 1) {
			logData = !logData;
			
			if (logData) {
				logger->startLogger();
			} else {
				logger->endLogger();
			}
		}
		
		// Grip button => init controller
		if (buttonNr == 2 && value == 1 && !calibratedAvatar) {
			log(Info, "Init Controller");
			initController();
		}
	}
	
	void initButtons() {
		VrPoseState controller;
		
		for (int i = 0; i < 16; ++i) {
			controller = VrInterface::getController(i);
			
			if (controller.trackedDevice == TrackedDevice::Controller)
				Gamepad::get(i)->Button = gamepadButton;
		}
		initedButtons = true;
	}
#endif
	
	void update() {
		float t = (float)(System::time() - startTime);
		double deltaT = t - lastTime;
		lastTime = t;
		
		// Move position of camera based on WASD keys
		float cameraMoveSpeed = 4.f;
		if (S) cameraPos -= camForward * (float) deltaT * cameraMoveSpeed;
		if (W) cameraPos += camForward * (float) deltaT * cameraMoveSpeed;
		if (A) cameraPos += camRight * (float)deltaT * cameraMoveSpeed;
		if (D) cameraPos -= camRight * (float)deltaT * cameraMoveSpeed;
		
		Graphics4::begin();
		Graphics4::clear(Graphics4::ClearColorFlag | Graphics4::ClearDepthFlag, Graphics1::Color::Black, 1.0f, 0);
		Graphics4::setPipeline(pipeline);
		
#ifdef KORE_STEAMVR
		VrInterface::begin();
		SensorState state;
		
		if (!initedButtons)
			initButtons();
		
		VrPoseState controller;
		for (int i = 0; i < numOfEndEffectors; ++i)
			if (endEffector[i]->getTrackerIndex() != -1) {
				controller = VrInterface::getController(endEffector[i]->getTrackerIndex());
				
				// Get controller position and rotation
				desPosition[i] = controller.vrPose.position;
				desRotation[i] = controller.vrPose.orientation;
				
				executeMovement(i);
			}
		
		// Render for both eyes
		for (int j = 0; j < 2; ++j) {
			VrInterface::beginRender(j);
			
			Graphics4::clear(Graphics4::ClearColorFlag | Graphics4::ClearDepthFlag, Graphics1::Color::Black, 1.0f, 0);
			
			state = VrInterface::getSensorState(j);
			
			renderAvatar(state.pose.vrPose.eye, state.pose.vrPose.projection);
			
			if (renderTrackerAndController) renderTracker();
			
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
		
		if (renderTrackerAndController) renderTracker();
		
		if (renderAxisForEndEffector) renderCSForEndEffector();
		
		if (renderRoom) {
			if (!firstPersonMonitor) renderLivingRoom(V, P);
			else renderLivingRoom(state.pose.vrPose.eye, state.pose.vrPose.projection);
		}
#else
		
		// Read line
		float scaleFactor;
		if (logger->readData(numOfEndEffectors, currentGroup[currentFile], desPosition, desRotation, scaleFactor)) {
			if (!calibratedAvatar) {
				avatar->setScale(scaleFactor);
				calibrate();
				calibratedAvatar = true;
			}
			
			for (int i = 0; i < numOfEndEffectors; ++i) executeMovement(i);
			
		} else {
			currentFile++;
			calibratedAvatar = false;
			
			// Evaluation?
            /*if (loop >= 0) {
                if (eval) logger->saveEvaluationData(avatar);
                // log(Kore::Info, "%i more iterations!", loop);
                log(Kore::Info, "%s\t%i\t%f", currentGroup[currentFile], ikMode, evalValue[ikMode]);
                loop--;
                
                if (eval && loop < 0) {
                    logger->endEvaluationLogger();
                    
                    if (currentFile >= evalFilesInGroup - 1 && ikMode >= 5 && evalSteps <= 1)
                        exit(0);
                    else {
                        if (evalSteps <= 1) {
                            evalValue[ikMode] = evalInitValue[ikMode];
                            evalSteps = evalStepsInit;
                            ikMode++;
                        } else {
                            evalValue[ikMode] += evalStep;
                            evalSteps--;
                        }
                        
                        if (ikMode > 5) {
                            ikMode = 0;
                            currentFile++;
                        }
                        
                        loop = 0;
                        logger->startEvaluationLogger();
                    }
                }
                
                if (loop >= 0)
                    initVars();
            }*/
        }
		
		// Get projection and view matrix
		mat4 P = getProjectionMatrix();
		mat4 V = getViewMatrix();
		
		renderAvatar(V, P);
		
		if (renderTrackerAndController) renderTracker();
		
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
				Kore::log(Kore::LogLevel::Info, "Position: (%f, %f, %f)", cameraPos.x(), cameraPos.y(), cameraPos.z());
				Kore::log(Kore::LogLevel::Info, "Looking at: (%f, %f %f %f)", camForward.x(), camForward.y(), camForward.z(), camForward.w());
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
		avatar = new Avatar("avatar/avatar_skeleton_headless.ogex", "avatar/", structure);
#else
		avatar = new Avatar("avatar/avatar_skeleton.ogex", "avatar/", structure);
#endif
		
		initRot = Kore::Quaternion(0, 0, 0, 1);
		initRot.rotate(Kore::Quaternion(vec3(1, 0, 0), -Kore::pi / 2.0));
		initRot.normalize();
		initRotInv = initRot.invert();
		
		vec3 initPos = initTrans * vec4(0, 0, 0, 1);
		initTrans = mat4::Translation(initPos.x(), initPos.y(), initPos.z()) * initRot.matrix().Transpose();
		initTransInv = initTrans.Invert();
		
		// Set camera initial position and orientation
		cameraPos = vec3(3.0, 3.5, 0.3);
		Kore::Quaternion q1(vec3(0.0f, 1.0f, 0.0f), Kore::pi / 2.0f);
		Kore::Quaternion q2(vec3(1.0f, 0.0f, 0.0f), -Kore::pi / 6.0f);
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
			livingRoom->M = mat4::Translation(-0.7, 0, 0) * livingRoomRot.matrix().Transpose();
		}
		
		logger = new Logger();
		if (eval) logger->startEvaluationLogger();
		
		endEffector = new EndEffector*[5];
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
	
	System::start();
	
	return 0;
}
