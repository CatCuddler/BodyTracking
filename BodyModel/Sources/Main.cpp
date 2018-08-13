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
	const int numOfEndEffectors = sizeof(tracker) / sizeof(*tracker);
	vec3 desPosition[numOfEndEffectors], trackerPosition[numOfEndEffectors];
	Kore::Quaternion desRotation[numOfEndEffectors], trackerRotation[numOfEndEffectors];
	
	Logger* logger;
	bool logData = false;
	int line = 0;
	int loop = 0;
	
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
	bool initedButtons = false;
#endif
	
	void renderTracker(int i) {
		cubes[i]->M = mat4::Translation(trackerPosition[i].x(), trackerPosition[i].y(), trackerPosition[i].z()) * trackerRotation[i].matrix().Transpose();
		Graphics4::setMatrix(mLocation, cubes[i]->M);
		cubes[i]->render(tex);
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
	
	void calibrateTracker(int i) {
		vec3 sollPos, istPos = desPosition[i];
		mat4 sollRot, istRot = desRotation[i].matrix();
		BoneNode* bone = avatar->getBoneWithIndex(tracker[i]->boneIndex);
		
		sollRot = initRot.rotated(bone->getOrientation()).matrix();
		sollPos = bone->getPosition();
		sollPos = initTrans * vec4(sollPos.x(), sollPos.y(), sollPos.z(), 1);
		
		tracker[i]->offsetPosition = (mat4::Translation(istPos.x(), istPos.y(), istPos.z()) * sollRot.Transpose()).Invert() * mat4::Translation(sollPos.x(), sollPos.y(), sollPos.z()) * vec4(0, 0, 0, 1);
		tracker[i]->offsetRotation = matrixToQuaternion(sollRot * istRot.Transpose());
	}
	
	void executeMovement(int i) {
		// set Tracker-Position
		trackerPosition[i] = desPosition[i];
		trackerRotation[i] = desRotation[i];
		
		// Add offset to endeffector
		Kore::Quaternion finalRot = desRotation[i].rotated(tracker[i]->offsetRotation);
		vec3 finalPos = mat4::Translation(desPosition[i].x(), desPosition[i].y(), desPosition[i].z()) * finalRot.matrix().Transpose() * mat4::Translation(tracker[i]->offsetPosition.x(), tracker[i]->offsetPosition.y(), tracker[i]->offsetPosition.z()) * vec4(0, 0, 0, 1);
		
		if (logData) logger->saveData(finalPos, finalRot);
		
		// Transform desired position to the character local coordinate system
		finalRot = initRotInv.rotated(finalRot);
		finalPos = initTransInv * vec4(finalPos.x(), finalPos.y(), finalPos.z(), 1);
		
		if (tracker[i]->boneIndex == tracker[rootIndex]->boneIndex)
			avatar->setFixedPositionAndOrientation(tracker[i]->boneIndex, finalPos, finalRot);
		else
			avatar->setDesiredPositionAndOrientation(tracker[i]->boneIndex, finalPos, finalRot);
	}
	
	
#ifdef KORE_STEAMVR
	void calibrateAvatar();
	
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
						log(Info, "leftFootTrackerIndex: %i -> %i", tracker[3]->trackerIndex, i);
						tracker[3]->setTrackerIndex(i);
					}
					else {
						log(Info, "rightFootTrackerIndex: %i -> %i", tracker[4]->trackerIndex, i);
						tracker[4]->setTrackerIndex(i);
					}
				}
				else {
					//back tracker
					log(Info, "backTrackerIndex: %i -> %i", tracker[2]->trackerIndex, i);
					tracker[2]->setTrackerIndex(i);
				}
			}
			else if (controller.trackedDevice == TrackedDevice::Controller) {
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
			}
		}
	}
	
	void gamepadButton(int buttonNr, float value) {
		// Menu button => calibrating
		if (buttonNr == 1 && value == 1) {
			if (!currentUserHeight)
				initController();
			
			if (!calibratedAvatar) {
				calibrateAvatar();
				calibratedAvatar = true;
				
				log(Info, "Calibrate avatar");
			} else {
				calibratedAvatar = false;
				
				log(Info, "Release avatar");
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
	
	void calibrateAvatar() {
		SensorState state = VrInterface::getSensorState(0);
		vec3 hmdPos = state.pose.vrPose.position; // z -> face, y -> up down
		
		// Set initial transformation
		initTrans = mat4::Translation(hmdPos.x(), 0, hmdPos.z());
		
		// Set initial orientation
		Kore::Quaternion hmdOrient = state.pose.vrPose.orientation;
		initRot = Kore::Quaternion(0, 0, 0, 1);
		initRot.rotate(Kore::Quaternion(vec3(0, 0, 1), -2 * Kore::acos(hmdOrient.y)));
		initRotInv = initRot.invert();
		
		const mat4 hmdOffset = mat4::Translation(0, 0.2f, 0);
		avatar->M = initTrans * initRot.matrix().Transpose() * hmdOffset;
		initTransInv = (initTrans * initRot.matrix().Transpose() * hmdOffset).Invert();
		
		// calibrate to T-Pose
		for (int i = 0; i < numOfEndEffectors; ++i) {
			VrPoseState controller = VrInterface::getController(tracker[i]->trackerIndex);
			
			desPosition[i] = controller.vrPose.position;
			desRotation[i] = controller.vrPose.orientation;
			
			calibrateTracker(i);
		}
		
		if (logData) {
			vec4 initPos = initTrans * vec4(0, 0, 0, 1);
			logger->saveInitTransAndRot(vec3(initPos.x(), initPos.y(), initPos.z()), initRot);
		}
	}
#endif
	
	void initVars() {
		// init & calibrate avatar
		avatar = new Avatar("avatar/avatar_skeleton.ogex", "avatar/", structure);
		calibratedAvatar = true; // recorded Data are always calibrated!
		
		currentFile->calibrated(false);
		
		// todo: entfernen wenn alle alten Daten neu aufgezeichnet wurden
		if (!currentFile->isCalibrated)
			avatar->setScale(1.0744f);
		
		vec3 initPos = vec3(0, 0, 0);
		
		log(Info, "Read data from file %s", currentFile->initialTransFilename);
		logger->readInitTransAndRot(currentFile->initialTransFilename, &initPos, &initRot);
		
		initRot.normalize();
		initRotInv = initRot.invert();
		
		initTrans = mat4::Translation(initPos.x(), initPos.y(), initPos.z()) * initRot.matrix().Transpose();
		initTransInv = initTrans.Invert();
		
		line = 0;
	}
	
	void update() {
		float t = (float)(System::time() - startTime);
		double deltaT = t - lastTime;
		lastTime = t;
		
		// Move position of camera based on WASD keys, and XZ keys for up and down
		const float moveSpeed = 0.1f;
		if (A) cameraPosition.x() -= moveSpeed;
		else if (D) cameraPosition.x() += moveSpeed;
		if (Z) cameraPosition.y() += moveSpeed;
		else if (X) cameraPosition.y() -= moveSpeed;
		if (S) cameraPosition.z() += moveSpeed;
		else if (W) cameraPosition.z() -= moveSpeed;
		
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
			if (tracker[i]->trackerIndex != -1) {
				controller = VrInterface::getController(tracker[i]->trackerIndex);
				
				// Get controller position and rotation
				desPosition[i] = controller.vrPose.position;
				desRotation[i] = controller.vrPose.orientation;
				
				if (calibratedAvatar)
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
			int i = 0;
			while (i < numOfEndEffectors && renderTrackerAndController && cubes[i] != nullptr) {
				renderTracker(i);
				i++;
			}
			
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
		
		// Animate avatar
		avatar->animate(tex, deltaT);
		
		// Render tracker
		int i = 0;
		while (i < numOfEndEffectors && renderTrackerAndController && cubes[i] != nullptr) {
			renderTracker(i);
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
			// Calibration
			// todo: entfernen wenn alle alten Daten neu aufgezeichnet wurden
			if (line < numOfEndEffectors)
				for (int i = 0; i < numOfEndEffectors; ++i)
					if (!currentFile->isCalibrated) {
						calibrateTracker(i);
						
						if (i == numOfEndEffectors - 1)
							currentFile->calibrated();
					}
			
			executeMovement(rootIndex); // todo: rootIndex löschen und root immer auf Pos 1 setzen, sodass Wurzel immer als erstes bewegt wird!!! Dies muss auch noch im Live-Mode geändert werden!
			for (int i = 0; i < numOfEndEffectors; ++i)
				if (i != rootIndex)
					executeMovement(i);
			
			line += numOfEndEffectors;
		} else {
			if (loop >= 0) {
				if (eval) logger->saveEvaluationData(avatar);
				initVars();
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
		
		// Animate avatar
		avatar->animate(tex, deltaT);
		
		// Render tracker
		int i = 0;
		while (i < numOfEndEffectors && renderTrackerAndController && cubes[i] != nullptr) {
			renderTracker(i);
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
		
		BoneNode* bone = avatar->getBoneWithIndex(rootIndex);
		
		bone->transform = mat4::Translation(0, 0.9f, 0);
		bone->quaternion = Kore::Quaternion(vec3(1, 0, 0), -0.4f);
		bone->quaternion.normalize();
		bone->local = bone->transform * bone->quaternion.matrix().Transpose();
		
#endif
		
		// camera
		cameraPosition = vec3(-1.1, 1.6, 4.5);
		cameraRotation.rotate(Kore::Quaternion(vec3(0, 1, 0), Kore::pi / 2.0f));
		cameraRotation.rotate(Kore::Quaternion(vec3(1, 0, 0), -Kore::pi / 6.0f));
		
		for (int i = 0; i < numOfEndEffectors; ++i)
			cubes[i] = new MeshObject("cube.ogex", "", structure, 0.05f);
		
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
