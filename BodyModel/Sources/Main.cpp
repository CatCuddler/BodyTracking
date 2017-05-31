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

#ifdef KORE_STEAMVR
#include <Kore/Vr/VrInterface.h>
#include <Kore/Vr/SensorState.h>
#endif

using namespace Kore;
using namespace Kore::Graphics4;

namespace {
	
	const int width = 1024;
	const int height = 768;
	
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
	bool rotateZ = false;
	int mousePressX, mousePressY = 0;
	
	MeshObject* cube;
	MeshObject* avatar;
	
	vec3 playerPosition = vec3(0, 0.7, 1.5);
	vec3 globe = vec3(0, 0, 0);
	int frame = 0;
	
	float angle = 0;
	vec3 desPos1 = vec4(0, 0, 0);
	vec3 desPos2 = vec4(0, 0, 0);

	mat4 T = mat4::Identity();
	mat4 initTrans = mat4::Identity();
	mat4 initRot = mat4::Identity();
	
	bool initCharacter = false;

	void update() {
		float t = (float)(System::time() - startTime);
		double deltaT = t - lastTime;
		lastTime = t;
		
		const float speed = 0.05f;
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

		VrInterface::begin();
		SensorState state;

		if (!initCharacter) {
			float currentAvatarHeight = avatar->getHeight();

			state = VrInterface::getSensorState(0);
			vec3 hmdPos = state.pose.vrPose.position; // z -> face, y -> up down
			float currentUserHeight = hmdPos.y();

			float scale = currentUserHeight / currentAvatarHeight;
//			avatar->setScale(scale);

			// Set initial transformation
			initTrans = mat4::Translation(hmdPos.x(), 0, hmdPos.z());
			
			// Set initial orientation
			state = VrInterface::getSensorState(0);
			Quaternion hmdOrient = state.pose.vrPose.orientation;
			float zAngle = 2 * Kore::acos(hmdOrient.y);
			initRot *= mat4::RotationZ(-zAngle);

			avatar->M = initTrans * initRot;
			T = (initTrans * initRot).Invert();

			log(Info, "current avatar height %f, currend user height %f, scale %f", currentAvatarHeight, currentUserHeight, scale);

			initCharacter = true;
		}

		// Get controller position
		VrPoseState controller = VrInterface::getController(4);
		vec3 desPosition = controller.vrPose.position;
		cube->M = mat4::Translation(desPosition.x(), desPosition.y(), desPosition.z());

		vec4 finalPos = T * vec4(desPosition.x(), desPosition.y(), desPosition.z(), 1);
		//avatar->setDesiredPosition(53, vec3(finalPos.x(), finalPos.y(), finalPos.z()));		// Left foot 49, right foot 53
		avatar->setDesiredPosition(29, vec3(finalPos.x(), finalPos.y(), finalPos.z()));			// Left hand 10, right hand 29
		
		for (int eye = 0; eye < 2; ++eye) {
			VrInterface::beginRender(eye);

			Graphics4::clear(Graphics4::ClearColorFlag | Graphics4::ClearDepthFlag, Graphics1::Color::Black, 1.0f, 0);

			state = VrInterface::getSensorState(eye);
			Graphics4::setMatrix(vLocation, state.pose.vrPose.eye);
			Graphics4::setMatrix(pLocation, state.pose.vrPose.projection);

			// Render
			Graphics4::setMatrix(mLocation, avatar->M);
			avatar->animate(tex, deltaT);
			Graphics4::setMatrix(mLocation, cube->M);
			cube->render(tex);

			VrInterface::endRender(eye);
		}

		VrInterface::warpSwap();

		Graphics4::restoreRenderTarget();
		Graphics4::clear(Graphics4::ClearColorFlag | Graphics4::ClearDepthFlag, Graphics1::Color::Black, 1.0f, 0);

		Graphics4::setMatrix(vLocation, state.pose.vrPose.eye);
		Graphics4::setMatrix(pLocation, state.pose.vrPose.projection);

		// Render
		Graphics4::setMatrix(mLocation, avatar->M);
		avatar->animate(tex, deltaT);
		Graphics4::setMatrix(mLocation, cube->M);
		cube->render(tex);

		//cube->drawVertices(cube->M, state.pose.vrPose.eye, state.pose.vrPose.projection, width, height);
		avatar->drawJoints(avatar->M, state.pose.vrPose.eye, state.pose.vrPose.projection, width, height, true);

#else
		// Scale test
		if (!initCharacter) {
			avatar->setScale(0.8);
			avatar->M = initTrans * initRot;
			initCharacter = true;
		}
		
		// projection matrix
		mat4 P = mat4::Perspective(45, (float)width / (float)height, 0.01f, 1000);
		
		// view matrix
		vec3 lookAt = playerPosition + vec3(0, 0, -1);
		mat4 V = mat4::lookAt(playerPosition, lookAt, vec3(0, 1, 0));
		V *= mat4::Rotation(globe.x(), globe.y(), globe.z());
		
		Graphics4::setMatrix(vLocation, V);
		Graphics4::setMatrix(pLocation, P);
		
		Graphics4::setMatrix(mLocation, cube->M);
		cube->render(tex);
		
		Graphics4::setMatrix(mLocation, avatar->M);
		/*avatar->setAnimation(frame);
		frame++;
		if (frame > 200) frame = 0;*/                                                                                                                                                            
		avatar->animate(tex, deltaT);
		
		angle += 0.05f;
		float radius = 0.2;
		desPos1 = vec3(-0.2 + radius * Kore::cos(angle), -0.2, 0.3 + radius * Kore::sin(angle));
		//desPos1 = vec3(-0.2, -1, 0.3);
		//desPos1 = vec3(-0.08, 0, 1);
		avatar->setDesiredPosition(53, desPos1);		// Left foot 49, right foot 53
		desPos2 = vec3(0.2 + radius * Kore::cos(angle), -0.3, 1.1 + radius * Kore::sin(angle));
		//desPos2 = vec3(0.2, -0.2, 1.1 + radius * Kore::sin(angle));
		avatar->setDesiredPosition(10, desPos2);		// Left hand 10, right hand 29
		
		//cube->drawVertices(cube->M, V, P, width, height);
		avatar->drawJoints(avatar->M, V, P, width, height, true);
#endif


		Graphics4::end();
		Graphics4::swapBuffers();
	}
	
	void keyDown(KeyCode code, wchar_t character) {
		switch (code) {
			case Kore::Key_Left:
			case Kore::Key_A:
				left = true;
				break;
			case Kore::Key_Right:
			case Kore::Key_D:
				right = true;
				break;
			case Kore::Key_Down:
				down = true;
				break;
			case Kore::Key_Up:
				up = true;
				break;
			case Kore::Key_W:
				forward = true;
				break;
			case Kore::Key_S:
				backward = true;
				break;
			case Kore::Key_X:
				rotateX = true;
				break;
			case Kore::Key_Z:
				rotateZ = true;
				break;
			case Kore::Key_R:
#ifdef KORE_STEAMVR
				VrInterface::resetHmdPose();
#endif
				break;
			case Key_L:
				Kore::log(Kore::LogLevel::Info, "Position: (%.2f, %.2f, %.2f)", playerPosition.x(), playerPosition.y(), playerPosition.z());
				Kore::log(Kore::LogLevel::Info, "Rotation: (%.2f, %.2f, %.2f)", globe.x(), globe.y(), globe.z());
			default:
				break;
		}
	}
	
	void keyUp(KeyCode code, wchar_t character) {
		switch (code) {
			case Kore::Key_Left:
			case Kore::Key_A:
				left = false;
				break;
			case Kore::Key_Right:
			case Kore::Key_D:
				right = false;
				break;
			case Kore::Key_Down:
				down = false;
				break;
			case Kore::Key_Up:
				up = false;
				break;
			case Kore::Key_W:
				forward = false;
				break;
			case Kore::Key_S:
				backward = false;
				break;
			case Kore::Key_X:
				rotateX = false;
				break;
			case Kore::Key_Z:
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
		rotateX = true;
		rotateZ = true;
		mousePressX = x;
		mousePressY = y;
	}
	
	void mouseRelease(int windowId, int button, int x, int y) {
		rotateX = false;
		rotateZ = false;
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
		pipeline->compile();
		
		tex = pipeline->getTextureUnit("tex");
		
		pLocation = pipeline->getConstantLocation("P");
		vLocation = pipeline->getConstantLocation("V");
		mLocation = pipeline->getConstantLocation("M");
		
		cube = new MeshObject("cube.ogex", "", structure, 0.01f);
		//cube->M = mat4::Translation(2, 0, 0);
		avatar = new MeshObject("avatar/avatar_skeleton.ogex", "avatar/", structure);
		//avatar->M = mat4::Translation(-2, 0, 0);
		initRot = mat4::RotationX(-Kore::pi / 2.0);
		
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
