#include "pch.h"

#include <Kore/IO/FileReader.h>
#include <Kore/Graphics4/PipelineState.h>
#include <Kore/Graphics1/Color.h>
#include <Kore/Input/Keyboard.h>
#include <Kore/Input/Mouse.h>
#include <Kore/System.h>

#include "Avatar.h"
#include "LivingRoom.h"
#include "Logger.h"
#include "Settings.h"

#ifdef KORE_STEAMVR
#include <Kore/Vr/VrInterface.h>
#include <Kore/Vr/SensorState.h>
#include <Kore/Input/Gamepad.h>
#endif

using namespace Kore;
using namespace Kore::Graphics4;

namespace {
	const int numOfEndEffectors = sizeof(tracker) / sizeof(*tracker);
	
	Logger* logger;
	int line = 0;
	bool logData = false;
	
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
	Avatar *avatar;
	LivingRoom* livingRoom;
	
	mat4 initTrans = mat4::Identity();
	mat4 initTransInv = mat4::Identity();
	Kore::Quaternion initRot = Kore::Quaternion(0, 0, 0, 1);
	Kore::Quaternion initRotInv = Kore::Quaternion(0, 0, 0, 1);
	
	bool calibratedAvatar = false;
	
#ifdef KORE_STEAMVR
	float currentUserHeight;
#endif
	
	void renderTracker(MeshObject*& cube, vec3 desPosition, Kore::Quaternion desRotation) {
		// Render desired position
		cube->M = mat4::Translation(desPosition.x(), desPosition.y(), desPosition.z()) * desRotation.matrix().Transpose();
		Graphics4::setMatrix(mLocation, cube->M);
		cube->render(tex);
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
	
	void calibrate(EndEffector* endEffector, vec3 istPos, Kore::Quaternion istRot) {
		vec3 sollPos, diffPos;
		mat4 sollRot, diffRot;
		// todo: im live Betrieb soll der Avatar still in der Mitte in T-Pose stehen, man sieht nur die Endeffektoren die sich bewegen. dann geht man zu dem Avatar und stellt sich "in ihn rein" und drückt den Kalibrierungs-Button. Danach fängt der Avatar sich an zu bewegen!
		BoneNode* bone = avatar->getBoneWithIndex(endEffector->boneIndex);
		
		sollRot = initRot.rotated(bone->getOrientation()).matrix();
		diffRot = sollRot * istRot.matrix().Transpose();
		
		sollPos = bone->getPosition();
		sollPos = initTrans * vec4(sollPos.x(), sollPos.y(), sollPos.z(), 1);
		diffPos = (mat4::Translation(istPos.x(), istPos.y(), istPos.z()) * sollRot.Transpose()).Invert() * mat4::Translation(sollPos.x(), sollPos.y(), sollPos.z()) * vec4(0, 0, 0, 1);
		
		endEffector->offsetPosition = diffPos;
		endEffector->offsetRotation = matrixToQuaternion(diffRot);
		
		calibratedAvatar = true;
	}
	
	void executeMovement(EndEffector* endEffector, vec3& desPosition, Kore::Quaternion& desRotation) {
		// Add offset to endeffector
		desRotation.rotate(endEffector->offsetRotation);
		desPosition = mat4::Translation(desPosition.x(), desPosition.y(), desPosition.z()) * desRotation.matrix().Transpose() * mat4::Translation(endEffector->offsetPosition.x(), endEffector->offsetPosition.y(), endEffector->offsetPosition.z()) * vec4(0, 0, 0, 1);
		
		if (logData) logger->saveData(desPosition, desRotation);
		
		// avatar movement
		if (calibratedAvatar) {
			// Transform desired position to the character local coordinate system
			vec3 finalPos = initTransInv * vec4(desPosition.x(), desPosition.y(), desPosition.z(), 1);
			Kore::Quaternion finalRot = initRotInv.rotated(desRotation);
			
			if (endEffector->boneIndex == tracker[rootIndex]->boneIndex) {
				avatar->setFixedPositionAndOrientation(endEffector->boneIndex, finalPos, finalRot);
				
				initTrans = mat4::Translation(desPosition.x(), 0, desPosition.z()) * initRot.matrix().Transpose();
				initTransInv = initTrans.Invert();
			} else
				avatar->setDesiredPositionAndOrientation(endEffector->boneIndex, finalPos, finalRot);
		}
	}
	
	
#ifdef KORE_STEAMVR
	void calibrateAvatar();

	void gamepadButton(int buttonNr, float value) {
		// Menu button => calibrating
		if (buttonNr == 1) {
			if (value == 1) {
				calibrateAvatar();
				
				log(Info, "Calibrate avatar");
			}
		}
		
		// Trigger button => logging
		if (buttonNr == 33) {
			if (value == 1) {
				logData = true;

				vec4 initPos = initTrans * vec4(0, 0, 0, 1);
				logger->saveInitTransAndRot(vec3(initPos.x(), initPos.y(), initPos.z()), initRot);

				log(Info, "Start logging data.");
			} else {
				logData = false;
				log(Info, "Stop recording data.");
			}
		}
		
		// Grip button
		if (buttonNr == 2) {
			if (value == 1) {
				// todo: Avatar "loslassen" (bleibt im Raum stehen und nur die Endeffektoren bewegen sich - notwendig falls man Kalibrierung noch mal machen will)
				calibratedAvatar = false;
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
					// Foot tracker (if y pos is in the lower quarter of the body)
					if (trackerTransPos.x() > 0) {
						log(Info, "leftFootTrackerIndex: %i -> %i", tracker[3]->trackerIndex, i);
						tracker[3]->setTrackerIndex(i);
					} else {
						log(Info, "rightFootTrackerIndex: %i -> %i", tracker[4]->trackerIndex, i);
						tracker[4]->setTrackerIndex(i);
					}
				} else {
					//back tracker
					log(Info, "backTrackerIndex: %i -> %i", tracker[2]->trackerIndex, i);
					tracker[2]->setTrackerIndex(i);
				}
			} else if (controller.trackedDevice == TrackedDevice::Controller) {
				//Hand tracker
				vec3 trackerPos = controller.vrPose.position;
				vec4 trackerTransPos = initTransInv * vec4(trackerPos.x(), trackerPos.y(), trackerPos.z(), 1);
				
				if (trackerTransPos.x() > 0) {
					log(Info, "leftHandTrackerIndex: %i -> %i", tracker[0]->trackerIndex, i);
					tracker[0]->setTrackerIndex(i);
				}
				else {
					log(Info, "rightHandTrackerIndex: %i -> %i", tracker[1]->trackerIndex, i);
					tracker[1]->setTrackerIndex(i);
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
		
		float scale = currentUserHeight / currentAvatarHeight; // todo: / 161.3f * 173.3f; // : ø Augenhöhe * ø Körperhöhe = 1.0744f
		avatar->setScale(scale);
		
		// Set initial transformation
		initTrans = mat4::Translation(hmdPos.x(), 0, hmdPos.z());
		
		// Set initial orientation
		Kore::Quaternion hmdOrient = state.pose.vrPose.orientation;
		initRot = Kore::Quaternion(0, 0, 0, 1);
		initRot.rotate(Kore::Quaternion(vec3(1, 0, 0), -Kore::pi / 2.0)); // Make the avatar stand on the feet
		initRot.rotate(Kore::Quaternion(vec3(0, 0, 1), -2 * Kore::acos(hmdOrient.y)));
		initRotInv = initRot.invert();
		
		const mat4 hmdOffset = mat4::Translation(0, 0.2f, 0);
		avatar->M = initTrans * initRot.matrix().Transpose() * hmdOffset;
		initTransInv = (initTrans * initRot.matrix().Transpose() * hmdOffset).Invert();
		
		log(Info, "current avatar height %f, current user height %f, scale %f", currentAvatarHeight, currentUserHeight, scale);
		
		// calibrate to T-Pose
		for (int i = 0; i < numOfEndEffectors; ++i) {
			VrPoseState controller = VrInterface::getController(tracker[i]->trackerIndex);
			
			// todo: vec3 finalPos = initTransInv * vec4(desPosition.x(), desPosition.y(), desPosition.z(), 1);
			// Kore::Quaternion finalRot = initRotInv.rotated(desRotation);
			
			calibrate(tracker[i], controller.vrPose.position, controller.vrPose.orientation);
			
			/* todo: if (i == 2) {
			 initTrans = mat4::Translation(controller.vrPose.position.x(), 0, controller.vrPose.position.z()) * initRot.matrix().Transpose();
			 initTransInv = initTrans.Invert();
			 } */
		}
		
		initController();
		
		if (logData) {
			vec4 initPos = initTrans * vec4(0, 0, 0, 1);
			logger->saveInitTransAndRot(vec3(initPos.x(), initPos.y(), initPos.z()), initRot);
		}
	}
#endif
	
	void update() {
		vec3 desPosition[numOfEndEffectors];
		Kore::Quaternion desRotation[numOfEndEffectors];
		
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
		bool firstPersonMonitor = false;
		
		VrInterface::begin();
		SensorState state;
		
		VrPoseState controller;
		for (int i = 0; i < numOfEndEffectors; ++i)
			if (tracker[i]->trackerIndex != -1) {
				controller = VrInterface::getController(tracker[i]->trackerIndex);
				
				// Get controller position and rotation
				desPosition[i] = controller.vrPose.position;
				desRotation[i] = controller.vrPose.orientation;
				
				executeMovement(tracker[i], desPosition[i], desRotation[i]);
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
			int i = 0;
			while (i < numOfEndEffectors && renderTrackerAndController && cubes[i] != nullptr) {
				renderTracker(cubes[i], desPosition[i], desRotation[i]);
				i++;
			}
			
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
		int i = 0;
		while (i < numOfEndEffectors && renderTrackerAndController && cubes[i] != nullptr) {
			renderTracker(cubes[i], desPosition[i], desRotation[i]);
			i++;
		}
		
		// Render living room
		if (renderRoom) {
			if (!firstPersonMonitor) renderLivingRoom(V, P);
			else renderLivingRoom(state.pose.vrPose.eye, state.pose.vrPose.projection);
		}
		
#else
		// Read line
		if (logger->readData(line, numOfEndEffectors, currentFile->positionDataFilename, desPosition, desRotation)) {
			for (int i = 0; i < numOfEndEffectors; ++i) {
				// Calibration
				// todo: entfernen wenn alle alten Daten neu aufgezeichnet wurden
				if (!currentFile->isCalibrated) {
					calibrate(tracker[i], desPosition[i], desRotation[i]);
					
					if (i == numOfEndEffectors - 1)
						currentFile->calibrated();
				}
				
				executeMovement(tracker[i], desPosition[i], desRotation[i]);
			}
			
			line += numOfEndEffectors;
		} else {
			if (logData) logger->saveLogData("it", avatar->getAverageIkIteration()); // todo:remove!
			if (eval) logger->saveEvaluationData(avatar);
			
			line = 0;
		}
		
		// Get projection and view matrix
		mat4 P = getProjectionMatrix();
		mat4 V = getViewMatrix();
		Graphics4::setMatrix(vLocation, V);
		Graphics4::setMatrix(pLocation, P);
		Graphics4::setMatrix(mLocation, initTrans);
		
		// Animate avatar
		avatar->animate(tex, deltaT);
		
		// Render tracker
		int i = 0;
		while (i < numOfEndEffectors && renderTrackerAndController && cubes[i] != nullptr) {
			renderTracker(cubes[i], desPosition[i], desRotation[i]);
			i++;
		}
		
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
		
		// todo: entfernen wenn alle alten Daten neu aufgezeichnet wurden
		if (!currentFile->isCalibrated)
			avatar->setScale(1.0744f);
#endif
		
		// camera
		cameraPosition = vec3(-1.1, 1.6, 4.5);
		cameraRotation.rotate(Kore::Quaternion(vec3(0, 1, 0), Kore::pi / 2));
		cameraRotation.rotate(Kore::Quaternion(vec3(1, 0, 0), -Kore::pi / 6));
		
		initRot = Kore::Quaternion(0, 0, 0, 1);
		initRot.rotate(Kore::Quaternion(vec3(1, 0, 0), -Kore::pi / 2.0));
		
		for (int i = 0; i < numOfEndEffectors; ++i)
			cubes[i] = new MeshObject("cube.ogex", "", structure, 0.05);
		
		if (renderRoom) {
			loadLivingRoomShader();
			livingRoom = new LivingRoom("sherlock_living_room/sherlock_living_room.ogex", "sherlock_living_room/", structure_living_room, 1);
			Kore::Quaternion livingRoomRot = Kore::Quaternion(0, 0, 0, 1);
			livingRoomRot.rotate(Kore::Quaternion(vec3(1, 0, 0), -Kore::pi / 2.0));
			livingRoomRot.rotate(Kore::Quaternion(vec3(0, 0, 1), Kore::pi / 2.0));
			livingRoom->M = mat4::Translation(-0.7, 0, 0) * livingRoomRot.matrix().Transpose();
		}
		
		logger = new Logger();
		
#ifdef KORE_STEAMVR
		VrInterface::init(nullptr, nullptr, nullptr); // TODO: Remove
#else
		// init & calibrate avatar
		vec3 initPos = vec3(0, 0, 0);
		
		log(Info, "Read data from file %s", currentFile->initialTransFilename);
		logger->readInitTransAndRot(currentFile->initialTransFilename, &initPos, &initRot);
		
		initRot.normalize();
		initRotInv = initRot.invert();
		
		initTrans = mat4::Translation(initPos.x(), initPos.y(), initPos.z()) * initRot.matrix().Transpose();
		initTransInv = initTrans.Invert();
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
