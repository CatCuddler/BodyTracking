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
#include "RotationUtility.h"
#include "Logger.h"

#ifdef KORE_STEAMVR
#include <Kore/Vr/VrInterface.h>
#include <Kore/Vr/SensorState.h>
#endif

using namespace Kore;
using namespace Kore::Graphics4;

namespace {
	
#ifdef KORE_STEAMVR
	const int width = 2048;
	const int height = 1024;
#else
	const int width = 1024;
	const int height = 768;
#endif
	
	bool logData = true;

	double startTime;
	double lastTime;
	
	VertexStructure structure;
	Shader* vertexShader;
	Shader* fragmentShader;
	PipelineState* pipeline;
	
	// Uniform locations
	TextureUnit tex;
	ConstantLocation pLocation;
	ConstantLocation vLocation;
	ConstantLocation mLocation;
	
	bool left, right = false;
	bool down, up = false;
	bool forward, backward = false;
	bool rotateX = false;
	bool rotateY = false;
	bool rotateZ = false;
	int mousePressX, mousePressY = 0;
	
	MeshObject* cube1;
	MeshObject* cube2;
	MeshObject* avatar;
	
#ifdef KORE_STEAMVR
	vec3 globe = vec3(Kore::pi, 0, 0);
	vec3 playerPosition = vec3(0, 0, 0);
	
	int leftTrackerIndex = -1;
	int rightTrackerIndex = -1;
#else
	vec3 globe = vec3(0, 0, 0);
	vec3 playerPosition = vec3(0, 0.7, 1.5);
#endif
	
	float angle = 0;
	vec3 desPosition1 = vec3(0, 0, 0);
	vec3 desPosition2 = vec3(0, 0, 0);
	Quaternion desRotation1 = Quaternion(0, 0, 0, 1);
	Quaternion desRotation2 = Quaternion(0, 0, 0, 1);
	
	Quaternion initDesRotationLeftHand = Quaternion(0, 0, 0, 1);
	Quaternion initDesRotationRightHand = Quaternion(0, 0, 0, 1);
	
	mat4 invT = mat4::Identity();
	mat4 initT = mat4::Identity();
	const mat4 hmdOffset = mat4::Translation(0, 0.2f, 0);
	Quaternion initRot = Quaternion(0, 0, 0, 1);
	Quaternion initRotInv = Quaternion(0, 0, 0, 1);
	
	bool initCharacter = false;
	
	// Left foot 49, right foot 53, Left hand 10, right hand 29
	const int leftHandBoneIndex = 10;
	const int rightHandBoneIndex = 29;
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
				vec3 targetPosition = avatar->getBonePosition(leftHandBoneIndex);
				Quaternion targetRotation = avatar->getBoneGlobalRotation(leftHandBoneIndex);
				cube1->M = avatar->M * mat4::Translation(targetPosition.x(), targetPosition.y(), targetPosition.z()) * targetRotation.matrix().Transpose();
				Graphics4::setMatrix(mLocation, cube1->M);
				cube1->render(tex);
				
				targetPosition = avatar->getBonePosition(rightHandBoneIndex);
				targetRotation = avatar->getBoneGlobalRotation(rightHandBoneIndex);
				cube2->M = avatar->M * mat4::Translation(targetPosition.x(), targetPosition.y(), targetPosition.z()) * targetRotation.matrix().Transpose();
				Graphics4::setMatrix(mLocation, cube2->M);
				cube2->render(tex);
				break;
			}
			default:
				break;
		}
	}
	
	Kore::mat4 getProjectionMatrix() {
		mat4 P = mat4::Perspective(45, (float)width / (float)height, 0.01f, 1000);
		P.Set(0, 0, -P.get(0, 0));
		return P;
	}
	
	Kore::mat4 getViewMatrix() {
		vec3 lookAt = playerPosition + vec3(0, 0, -1);
		mat4 V = mat4::lookAt(playerPosition, lookAt, vec3(0, 1, 0));
		V *= mat4::Rotation(globe.x(), globe.y(), globe.z());
		return V;
	}
	
	void setDesiredPosition(Kore::vec3 desPosition, int boneIndex) {
		// Transform desired position to the character coordinate system
		vec4 finalPos = invT * vec4(desPosition.x(), desPosition.y(), desPosition.z(), 1);
		avatar->setDesiredPosition(boneIndex, finalPos);
	}

	void setDesiredPositionAndOrientation(Kore::vec3 &desPosition, Kore::Quaternion &desRotation, const int boneIndex) {
		// Transform desired position to the character coordinate system
		//vec4 finalPos = invT * vec4(desPosition.x(), desPosition.y(), desPosition.z(), 1);

		float handOffsetX = 0.05f;
		float handOffsetY = 0;// 0.05f;

		float rotOffsetY = Kore::pi / 4;
		
		Kore::Quaternion desRot = desRotation;
		if (boneIndex == rightHandBoneIndex) {
			desRot.rotate(initDesRotationRightHand);
			handOffsetX = -handOffsetX;
			desRot.rotate(Kore::Quaternion(Kore::vec3(0, 1, 0), -rotOffsetY));
		} else if (boneIndex == leftHandBoneIndex) {
			desRot.rotate(initDesRotationLeftHand);
			desRot.rotate(Kore::Quaternion(Kore::vec3(0, 1, 0), rotOffsetY));
		}
		desRotation = desRot;
		
		// Transform desired position to the hand bone
		Kore::mat4 curPos = mat4::Translation(desPosition.x(), desPosition.y(), desPosition.z()) * desRot.matrix().Transpose() * mat4::Translation(0, handOffsetY, 0);
		Kore::vec4 desPos = curPos * vec4(0, 0, 0, 1);
		desPosition = vec3(desPos.x(), desPos.y(), desPos.z());
		
		// Transform desired position to the character coordinate system
		vec4 finalPos = invT * vec4(desPos.x(), desPos.y(), desPos.z(), 1);
		Kore::Quaternion finalRot = initRotInv.rotated(desRotation);
		
		if (logData) {
			Logger::saveData(finalPos, finalRot);
		}
		
		avatar->setDesiredPositionAndOrientation(boneIndex, finalPos, finalRot);
	}
	
	void update() {
		float t = (float)(System::time() - startTime);
		double deltaT = t - lastTime;
		lastTime = t;
		
		const float speed = 0.01f;
		if (left) {
			playerPosition.x() -= speed;
		}
		if (right) {
			playerPosition.x() += speed;
		}
		if (forward) {
			playerPosition.z() += speed;
		}
		if (backward) {
			playerPosition.z() -= speed;
		}
		if (up) {
			playerPosition.y() += speed;
		}
		if (down) {
			playerPosition.y() -= speed;
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
			float currentUserHeight = hmdPos.y();
			
			//playerPosition.x() = -currentUserHeight * 0.5;
			playerPosition.y() = currentUserHeight * 1.5;
			playerPosition.z() = currentUserHeight * 0.5;
			
			float scale = currentUserHeight / currentAvatarHeight;
			avatar->setScale(scale);
			
			// Set initial transformation
			initT = mat4::Translation(hmdPos.x(), 0, hmdPos.z());
			
			initDesRotationLeftHand.rotate(Quaternion(vec3(0, 1, 0), -Kore::pi / 2));
			initDesRotationRightHand.rotate(Quaternion(vec3(0, 1, 0), Kore::pi / 2));
			
			// Set initial orientation
			Quaternion hmdOrient = state.pose.vrPose.orientation;
			float zAngle = 2 * Kore::acos(hmdOrient.y);
			initRot.rotate(Quaternion(vec3(0, 0, 1), -zAngle));
			
			initRotInv = initRot.invert();
			
			avatar->M = initT * initRot.matrix().Transpose() * hmdOffset;
			invT = (initT * initRot.matrix().Transpose() * hmdOffset).Invert();
			
			log(Info, "current avatar height %f, currend user height %f, scale %f", currentAvatarHeight, currentUserHeight, scale);
			
			// Get left and right tracker index
			VrPoseState controller;
			for (int i = 0; i < 16; ++i) {
				controller = VrInterface::getController(i);
				if (controller.trackedDevice == TrackedDevice::ViveTracker) {
					vec3 trackerPos = controller.vrPose.position;
					vec4 trackerTransPos = invT * vec4(trackerPos.x(), trackerPos.y(), trackerPos.z(), 1);
					if (trackerTransPos.x() > 0) {
						log(Info, "leftTrackerIndex: %i -> %i", leftTrackerIndex, i);
						leftTrackerIndex = i;
					} else {
						log(Info, "rightTrackerIndex: %i -> %i", rightTrackerIndex, i);
						rightTrackerIndex = i;
					}
					//leftTrackerIndex = -1;
					//rightTrackerIndex = i;
				}
			}
			
			initCharacter = true;
		}
		
		VrPoseState controller;
		/*for (int i = 0; i < 16; ++i) {
			controller = VrInterface::getController(i);
			if (controller.trackedDevice == TrackedDevice::ViveTracker) break;
		}*/
		if (leftTrackerIndex != -1) {
			controller = VrInterface::getController(leftTrackerIndex);

			// Get controller position
			desPosition1 = controller.vrPose.position;
			// Get cont1roller rotation
			desRotation1 = controller.vrPose.orientation;
			
			setDesiredPositionAndOrientation(desPosition1, desRotation1, leftHandBoneIndex);
		}

		if (rightTrackerIndex != -1) {
			controller = VrInterface::getController(rightTrackerIndex);

			// Get controller position
			desPosition2 = controller.vrPose.position;
			// Get controller rotation
			desRotation2 = controller.vrPose.orientation;
				
			setDesiredPositionAndOrientation(desPosition2, desRotation2, rightHandBoneIndex);
		}
		
		for (int eye = 0; eye < 2; ++eye) {
			VrInterface::beginRender(eye);
			
			Graphics4::clear(Graphics4::ClearColorFlag | Graphics4::ClearDepthFlag, Graphics1::Color::Black, 1.0f, 0);
			
			state = VrInterface::getSensorState(eye);
			Graphics4::setMatrix(vLocation, state.pose.vrPose.eye);
			Graphics4::setMatrix(pLocation, state.pose.vrPose.projection);
			
			// Render
			Graphics4::setMatrix(mLocation, avatar->M);
			avatar->animate(tex, deltaT);
			
			renderTracker();
			
			VrInterface::endRender(eye);
		}
		
		VrInterface::warpSwap();
		
		Graphics4::restoreRenderTarget();
		Graphics4::clear(Graphics4::ClearColorFlag | Graphics4::ClearDepthFlag, Graphics1::Color::Black, 1.0f, 0);
		
		// Render
		if (!firstPersonMonitor) {
			mat4 P = getProjectionMatrix();
			mat4 V = getViewMatrix();
			
			Graphics4::setMatrix(vLocation, V);
			Graphics4::setMatrix(pLocation, P);
		} else {
			Graphics4::setMatrix(vLocation, state.pose.vrPose.eye);
			Graphics4::setMatrix(pLocation, state.pose.vrPose.projection);
		}
		Graphics4::setMatrix(mLocation, avatar->M);
		avatar->animate(tex, deltaT);
		
		renderTracker();
		Graphics4::setPipeline(pipeline);
		
		//cube->drawVertices(cube->M, state.pose.vrPose.eye, state.pose.vrPose.projection, width, height);
		//avatar->drawJoints(avatar->M, state.pose.vrPose.eye, state.pose.vrPose.projection, width, height, true);
		
#else
		// Scale test
		if (!initCharacter) {
			avatar->setScale(0.8);
			
			initRotInv = initRot.invert();
			
			avatar->M = initT * initRot.matrix().Transpose();
			invT = (initT * initRot.matrix().Transpose()).Invert();
			initCharacter = true;
			
			initDesRotationLeftHand.rotate(Quaternion(vec3(0, 1, 0), -Kore::pi / 2));
			initDesRotationLeftHand.rotate(Quaternion(vec3(0, 0, 1), -Kore::pi / 2));
			initDesRotationRightHand.rotate(Quaternion(vec3(0, 1, 0), Kore::pi/2));
			initDesRotationRightHand.rotate(Quaternion(vec3(0, 0, 1), Kore::pi/2));
			
		}
		
		// projection matrix
		mat4 P = getProjectionMatrix();
		
		// view matrix
		mat4 V = getViewMatrix();
		
		Graphics4::setMatrix(vLocation, V);
		Graphics4::setMatrix(pLocation, P);
		
		Graphics4::setMatrix(mLocation, avatar->M);
		
		avatar->animate(tex, deltaT);
		
		angle += 0.05;
		float radius = 0.2;
		
		// Set foot position
		//desPosition2 = vec3(-0.2 + radius * Kore::cos(angle), 0.3 + radius * Kore::sin(angle), 0.2);
		//setDesiredPosition(desPosition2, 53); // Left foot 49, right foot 53
		
		// Set hand position
		radius = 0.1;
		//desPosition = vec3(0.2 + radius * Kore::cos(angle), 0.9 + radius * Kore::sin(angle), 0.2);
		//desPosition = vec3(0.2 + radius * Kore::cos(angle), 0.9, 0.2);
		
		
		// Set position and orientation for the left hand
		desPosition1 = vec3(0.2, 0.9, 0.4);
		desRotation1 = Quaternion(vec3(0, 0, 1), -angle);
		setDesiredPositionAndOrientation(desPosition1, desRotation1, leftHandBoneIndex);
		
		// Set position and orientation for the right hand
		//desPosition2 = vec3(-0.2, 0.9, 0.4);
		//desRotation2 = Quaternion(vec3(0, 0, 1), angle);
		//setDesiredPositionAndOrientation(desPosition2, desRotation2, rightHandBoneIndex);
		
		//cube->drawVertices(cube->M, V, P, width, height);
		//avatar->drawJoints(avatar->M, V, P, width, height, true);
		
		renderTracker();
		Graphics4::setPipeline(pipeline);
#endif
		
		
		Graphics4::end();
		Graphics4::swapBuffers();
	}
	
	void keyDown(KeyCode code) {
		switch (code) {
			case Kore::KeyLeft:
			case Kore::KeyA:
				left = true;
				break;
			case Kore::KeyRight:
			case Kore::KeyD:
				right = true;
				break;
			case Kore::KeyDown:
				down = true;
				break;
			case Kore::KeyUp:
				up = true;
				break;
			case Kore::KeyW:
				forward = true;
				break;
			case Kore::KeyS:
				backward = true;
				break;
			case Kore::KeyX:
				rotateX = true;
				break;
			case Kore::KeyY:
				rotateY = true;
				break;
			case Kore::KeyZ:
				rotateZ = true;
				break;
			case Kore::KeyR:
#ifdef KORE_STEAMVR
				VrInterface::resetHmdPose();
#endif
				break;
			case KeyL:
				Kore::log(Kore::LogLevel::Info, "Position: (%f, %f, %f)", playerPosition.x(), playerPosition.y(), playerPosition.z());
				Kore::log(Kore::LogLevel::Info, "Rotation: (%f, %f, %f)", globe.x(), globe.y(), globe.z());
				break;
			case KeyQ:
				System::stop();
				break;
			default:
				break;
		}
	}
	
	void keyUp(KeyCode code) {
		switch (code) {
			case Kore::KeyLeft:
			case Kore::KeyA:
				left = false;
				break;
			case Kore::KeyRight:
			case Kore::KeyD:
				right = false;
				break;
			case Kore::KeyDown:
				down = false;
				break;
			case Kore::KeyUp:
				up = false;
				break;
			case Kore::KeyW:
				forward = false;
				break;
			case Kore::KeyS:
				backward = false;
				break;
			case Kore::KeyX:
				rotateX = false;
				break;
			case Kore::KeyY:
				rotateY = false;
				break;
			case Kore::KeyZ:
				rotateZ = false;
				break;
			default:
				break;
		}
	}
	
	void mouseMove(int windowId, int x, int y, int movementX, int movementY) {
		float rotationSpeed = 0.01;
		
		if (rotateX) {
			globe.x() += (float)((mousePressX - x) * rotationSpeed);
			mousePressX = x;
		} else if (rotateZ) {
			globe.z() += (float)((mousePressY - y) * rotationSpeed);
			mousePressY = y;
		}
		
	}
	
	void mousePress(int windowId, int button, int x, int y) {
		//rotateX = true;
		//rotateZ = true;
		mousePressX = x;
		mousePressY = y;
	}
	
	void mouseRelease(int windowId, int button, int x, int y) {
		//rotateX = false;
		//rotateZ = false;
	}
	
	void init() {
		FileReader vs("shader.vert");
		FileReader fs("shader.frag");
		//FileReader vs("shader_lighting.vert");
		//FileReader fs("shader_lighting.frag");
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
		
		pLocation = pipeline->getConstantLocation("P");
		vLocation = pipeline->getConstantLocation("V");
		mLocation = pipeline->getConstantLocation("M");
		
		cube1 = new MeshObject("cube.ogex", "", structure, 0.05);
		cube2 = new MeshObject("cube.ogex", "", structure, 0.05);
#ifdef KORE_STEAMVR
		avatar = new MeshObject("avatar/avatar_skeleton_headless.ogex", "avatar/", structure);
#else
		avatar = new MeshObject("avatar/avatar_skeleton.ogex", "avatar/", structure);
#endif
		initRot.rotate(Quaternion(vec3(1, 0, 0), -Kore::pi / 2.0));
		
		Graphics4::setTextureAddressing(tex, Graphics4::U, Repeat);
		Graphics4::setTextureAddressing(tex, Graphics4::V, Repeat);
		
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
