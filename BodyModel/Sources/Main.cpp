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
int ikMode = 5; // 0: JT, 1: JPI, 2: DLS, 3: SVD, 4: DLS with SVD, 5: SDLS, 6: SDLS-Modified
int maxSteps[] = { 100, 100, 100, 100, 100, 100 };
float dMaxPos[] = { 0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f };
float dMaxRot[] = { 1.25f, 1.25f, 1.25f, 1.25f, 1.25f, 1.25f };
float lambda[] = { 0.25f, 0.01f, 0.18f, 0.1f, 0.18f, Kore::pi / 4, Kore::pi / 4 };

namespace {
	EndEffector** endEffector;
	vec3 desPosition[numOfEndEffectors], trackerPosition[numOfEndEffectors];
	Kore::Quaternion desRotation[numOfEndEffectors], trackerRotation[numOfEndEffectors];
	
	Logger* logger;
	bool logData = false;
	int line = 0;
	
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
	bool Z, X = false;
	
	vec4 camUp(0.0f, 1.0f, 0.0f, 0.0f);
	vec4 camForward(0.0f, 0.0f, 1.0f, 0.0f);
	vec4 camRight(1.0f, 0.0f, 0.0f, 0.0f);
	
	vec3 cameraPos(0, 0, 0);
	
	// Null terminated array of MeshObject pointers (Vive Controller and Tracker)
	MeshObject* viveObjects[] = { nullptr, nullptr, nullptr };
	Avatar *avatar;
	LivingRoom* livingRoom;
	
	mat4 initTrans = mat4::Identity();
	mat4 initTransInv = mat4::Identity();
	Kore::Quaternion initRot = Kore::Quaternion(0, 0, 0, 1);
	Kore::Quaternion initRotInv = Kore::Quaternion(0, 0, 0, 1);
	
	bool calibratedAvatar = false;
	
#ifdef KORE_STEAMVR
	float currentUserHeight;
	bool initedButtons = false;
	bool firstPersonMonitor = false;
#else
	int loop = 0;
#endif
	
	void renderTracker() {
		for(int i = 0; i < numOfEndEffectors; ++i) {
			Kore::mat4 M = mat4::Translation(trackerPosition[i].x(), trackerPosition[i].y(), trackerPosition[i].z()) * trackerRotation[i].matrix().Transpose();
			Graphics4::setMatrix(mLocation, M);
			
			// Tender a tracker for both feet and back
			if (i == hip || i == rightFoot || i == leftFoot) {
				viveObjects[0]->render(tex);
				// Render a controller for both hands
			} else if (i == rightHand || i == leftHand) {
				viveObjects[1]->render(tex);
			}
			
			// Render local coordinate system
			viveObjects[2]->render(tex);
		}
	}
	
	void renderCSForEndEffector() {
		for(int i = 0; i < numOfEndEffectors; ++i) {
			BoneNode* bone = avatar->getBoneWithIndex(endEffector[i]->getBoneIndex());
			
			Kore::Quaternion endEffectorRot = initRot.rotated(bone->getOrientation());
			vec3 endEffectorPos = bone->getPosition();;
			endEffectorPos = initTrans * vec4(endEffectorPos.x(), endEffectorPos.y(), endEffectorPos.z(), 1);
			
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
	
	Kore::mat4 getProjectionMatrix() {
		mat4 P = mat4::Perspective(45, (float)width / (float)height, 0.01f, 1000);
		P.Set(0, 0, -P.get(0, 0));
		
		return P;
	}
	
	Kore::mat4 getViewMatrix() {
		mat4 V = mat4::lookAlong(camForward.xyz(), cameraPos, vec3(0.0f, 1.0f, 0.0f));
		return V;
	}
	
	void executeMovement(int i) {
		trackerPosition[i] = desPosition[i];
		trackerRotation[i] = desRotation[i];
		
		if (calibratedAvatar) {
			// Add offset to endeffector
#ifdef KORE_STEAMVR
			Kore::Quaternion finalRot = desRotation[i].rotated(endEffector[i]->offsetRotation);
			vec3 finalPos = mat4::Translation(desPosition[i].x(), desPosition[i].y(), desPosition[i].z()) * finalRot.matrix().Transpose() * mat4::Translation(endEffector[i]->offsetPosition.x(), endEffector[i]->offsetPosition.y(), endEffector[i]->offsetPosition.z()) * vec4(0, 0, 0, 1);
#else
			Kore::Quaternion finalRot = desRotation[i];
			vec3 finalPos = desPosition[i];
#endif
			if (logData) logger->saveData(finalPos, finalRot);
			
			// Transform desired position to the character local coordinate system
			finalRot = initRotInv.rotated(finalRot);
			finalPos = initTransInv * vec4(finalPos.x(), finalPos.y(), finalPos.z(), 1);
			
			if (!i)
				avatar->setFixedPositionAndOrientation(endEffector[i]->getBoneIndex(), finalPos, finalRot);
			else
				avatar->setDesiredPositionAndOrientation(endEffector[i]->getBoneIndex(), finalPos, finalRot);
		}
	}
	
#ifdef KORE_STEAMVR
	Kore::Quaternion matrixToQuaternion(mat4 diffRot, int i = 0) {
		vec4 result;
		int j = i < 2 ? i + 1 : 0;
		int k = i > 0 ? i - 1 : 2;
		
		if (diffRot[i][i] >= diffRot[j][j] && diffRot[i][i] >= diffRot[k][k]) {
			result[i] = sqrtf(1 + diffRot[i][i] - diffRot[j][j] - diffRot[k][k] ) / 2;
			result[j] = (diffRot[j][i] + diffRot[i][j]) / (4 * result[i]);
			result[k] = (diffRot[k][i] + diffRot[i][k]) / (4 * result[i]);
			result[3] = (diffRot[k][j] - diffRot[j][k]) / (4 * result[i]);
			
			// check if w < 0?
			if (!(
				  (result.z() < nearNull || fabs(2 * (result.x() * result.y() - result.w() * result.z()) - diffRot[1][0]) < nearNull) &&
				  (result.y() < nearNull || fabs(2 * (result.x() * result.z() + result.w() * result.y()) - diffRot[2][0]) < nearNull) &&
				  (result.x() < nearNull || fabs(2 * (result.y() * result.z() - result.w() * result.x()) - diffRot[2][1]) < nearNull)
				  )) result.w() = -result.w();
			
			return Kore::Quaternion(result.x(), result.y(), result.z(), result.w());
		}
		
		return matrixToQuaternion(diffRot, i + 1);
	}
	
	void calibrateTracker(int i) {
		vec3 sollPos, istPos = desPosition[i];
		mat4 sollRot, istRot = desRotation[i].matrix();
		BoneNode* bone = avatar->getBoneWithIndex(endEffector[i]->getBoneIndex());
		
		sollRot = initRot.rotated(bone->getOrientation()).matrix();
		sollPos = bone->getPosition();
		sollPos = initTrans * vec4(sollPos.x(), sollPos.y(), sollPos.z(), 1);
		
		endEffector[i]->offsetPosition = (mat4::Translation(istPos.x(), istPos.y(), istPos.z()) * sollRot.Transpose()).Invert() * mat4::Translation(sollPos.x(), sollPos.y(), sollPos.z()) * vec4(0, 0, 0, 1);
		endEffector[i]->offsetRotation = matrixToQuaternion(sollRot * istRot.Transpose());
	}
	
	void setSize() {
		float currentAvatarHeight = avatar->getHeight();
		
		SensorState state = VrInterface::getSensorState(0);
		vec3 hmdPos = state.pose.vrPose.position; // z -> face, y -> up down
		currentUserHeight = hmdPos.y();
		
		// todo: float factor = 173.3f / 161.3f; // ø Körperhöhe / ø Augenhöhe = 1.0744f
		float scale = currentUserHeight / currentAvatarHeight;
		avatar->setScale(scale);
		
		log(Info, "current avatar height %f, current user height %f, scale %f", currentAvatarHeight, currentUserHeight, scale);
	}
	
	void initController() {
		VrPoseState controller;
		
		// set Avatar size
		setSize();
		
		// Get left and right tracker index
		for (int i = 0; i < 16; ++i) {
			controller = VrInterface::getController(i);
			
			if (controller.trackedDevice == TrackedDevice::ViveTracker) {
				vec3 trackerPos = controller.vrPose.position;
				vec4 trackerTransPos = initTransInv * vec4(trackerPos.x(), trackerPos.y(), trackerPos.z(), 1);
				
				if (trackerPos.y() < currentUserHeight / 4) {
					// Foot tracker (if y pos is in the lower quarter of the body)
					if (trackerTransPos.x() > 0) {
						log(Info, "rightFoot: %i -> %i", endEffector[rightHand]->trackerIndex, i);
						tracker[rightFoot]->setTrackerIndex(i);
					}
					else {
						log(Info, "rightFoot: %i -> %i", endEffector[rightFoot]->trackerIndex, i);
						tracker[rightHand]->setTrackerIndex(i);
					}
				}
				else {
					// Hip tracker
					log(Info, "hip: %i -> %i", endEffector[hip]->trackerIndex, i);
					tracker[hip]->setTrackerIndex(i);
				}
			}
			else if (controller.trackedDevice == TrackedDevice::Controller) {
				// Hand tracker
				vec3 trackerPos = controller.vrPose.position;
				vec4 trackerTransPos = initTransInv * vec4(trackerPos.x(), trackerPos.y(), trackerPos.z(), 1);
				
				if (trackerTransPos.x() > 0) {
					log(Info, "leftHand: %i -> %i", endEffector[leftHand]->trackerIndex, i);
					endEffector[leftHand]->setTrackerIndex(i);
				}
				else {
					log(Info, "rightHand: %i -> %i", endEffector[rightHand]->trackerIndex, i);
					endEffector[rightHand]->setTrackerIndex(i);
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
				// calibrate to T-Pose
				for (int i = 0; i < numOfEndEffectors; ++i) {
					VrPoseState controller = VrInterface::getController(endEffector[i]->trackerIndex);
					
					desPosition[i] = controller.vrPose.position;
					desRotation[i] = controller.vrPose.orientation;
					
					calibrateTracker(i);
				}
				
				log(Info, "Calibrate avatar");
				calibratedAvatar = true;
			} else {
				log(Info, "Release avatar");
				calibratedAvatar = false;
			}
		}
		
		// Trigger button => logging
		if (buttonNr == 33) {
			if (value == 1) {
				logData = true;
				log(Info, "Start logging data.");
			} else {
				logData = false;
				log(Info, "Stop recording data.");
			}
		}
		
		// Grip button => init Controller
		if (buttonNr == 2 && value == 1 && !calibratedAvatar) {
			initController();
			log(Info, "Init Controller");
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
	
	void initVars() {
		// init & calibrate avatar
		avatar->setScale(currentFile->scale);
		calibratedAvatar = true; // recorded Data are always calibrated!
		
		initRot.normalize();
		initRotInv = initRot.invert();
		
		initTrans = mat4::Translation(0, 0, 0) * initRot.matrix().Transpose();
		initTransInv = initTrans.Invert();
		
		line = 0;
	}
	
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
			if (endEffector[i]->trackerIndex != -1) {
				controller = VrInterface::getController(endEffector[i]->trackerIndex);
				
				// Get controller position and rotation
				desPosition[i] = controller.vrPose.position;
				desRotation[i] = controller.vrPose.orientation;
				
				executeMovement(i);
			}
		
		// Render for both eyes
		for (int j = 0; j < 2; ++j) {
			VrInterface::beginRender(j);
			
			Graphics4::setPipeline(pipeline);
			
			Graphics4::clear(Graphics4::ClearColorFlag | Graphics4::ClearDepthFlag, Graphics1::Color::Black, 1.0f, 0);
			
			state = VrInterface::getSensorState(j);
			Graphics4::setMatrix(vLocation, state.pose.vrPose.eye);
			Graphics4::setMatrix(pLocation, state.pose.vrPose.projection);
			
			// Animate avatar
			Graphics4::setMatrix(mLocation, initTrans);
			avatar->animate(tex, deltaT);
			
			// Render tracker
			if (renderTrackerAndController) renderTracker();
			
			// Rencer Coordinate System for End-Effectors
			if (renderAxisForEndEffector) renderCSForEndEffector();
			
			// Render living room
			renderLivingRoom(state.pose.vrPose.eye, state.pose.vrPose.projection);
			
			VrInterface::endRender(j);
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
		
		avatar->animate(tex, deltaT);
		
		if (renderTrackerAndController) renderTracker();
		
		if (renderAxisForEndEffector) renderCSForEndEffector();
		
		if (renderRoom) {
			if (!firstPersonMonitor) renderLivingRoom(V, P);
			else renderLivingRoom(state.pose.vrPose.eye, state.pose.vrPose.projection);
		}
#else
		// Read line
		if (logger->readData(line, numOfEndEffectors, currentFile->positionDataFilename, desPosition, desRotation)) {
			for (int i = 0; i < numOfEndEffectors; ++i)
				executeMovement(i);
			
			line += numOfEndEffectors;
		} else {
			if (loop >= 0) {
				if (eval) logger->saveEvaluationData(avatar);
				if (loop > 0) initVars();
				log(Kore::Info, "%i more iterations!", loop);
				loop--;
				
				/* if (loop < 0) {
				 if (eval) logger->endEvaluationLogger();
				 
				 // todo: remove after eval
				 if (ikMode < 6) {
				 ikMode++;
				 loop = 5;
				 }
				 if (eval) logger->startEvaluationLogger();
				 } */
			}
		}
		
		// Get projection and view matrix
		mat4 P = getProjectionMatrix();
		mat4 V = getViewMatrix();
		Graphics4::setMatrix(vLocation, V);
		Graphics4::setMatrix(pLocation, P);
		Graphics4::setMatrix(mLocation, initTrans);
		
		avatar->animate(tex, deltaT);
		
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
		// Load shader for living room
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
#else
		initVars();
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
