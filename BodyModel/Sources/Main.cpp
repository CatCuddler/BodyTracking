#include "pch.h"

#include <Kore/IO/FileReader.h>
#include <Kore/Graphics/Graphics.h>
#include <Kore/Graphics/Color.h>
#include <Kore/Input/Keyboard.h>
#include <Kore/Input/Mouse.h>
#include <Kore/System.h>
#include <Kore/Log.h>

#include "MeshObject.h"

using namespace Kore;

namespace {
	
	const int width = 1024;
	const int height = 768;
	
	double startTime;
	
	Shader* vertexShader;
	Shader* fragmentShader;
	Program* program;
	
	// Uniform locations
	TextureUnit tex;
	ConstantLocation pLocation;
	ConstantLocation vLocation;
	ConstantLocation mLocation;
	
	bool left, right = false;
	bool down, up = false;
	bool forward, backward = false;
	bool rotate = false;
	int mousePressX, mousePressY = 0;
	
	MeshObject* cube;
    MeshObject* avatar;
	
	vec3 playerPosition = vec3(0, 0, 50);
	vec3 globe = vec3(0, Kore::pi, Kore::pi);
	
	void update() {
		float t = (float)(System::time() - startTime);
		
		const float speed = 1;
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
		
		Graphics::begin();
		Graphics::clear(Graphics::ClearColorFlag | Graphics::ClearDepthFlag, Color::Black, 1.0f, 0);
		
		program->set();
		
		// projection matrix
		mat4 P = mat4::Perspective(45, (float)width / (float)height, 0.01f, 1000);
		
		// view matrix
		vec3 lookAt = playerPosition + vec3(0, 0, -1);
		mat4 V = mat4::lookAt(playerPosition, lookAt, vec3(0, 1, 0));
		V *= mat4::Rotation(globe.x(), globe.y(), globe.z());
		
		// model matrix
		//mat4 M = mat4::Identity();
		
		cube->render(tex);
        Graphics::setMatrix(mLocation, cube->M);
        
        avatar->render(tex);
        Graphics::setMatrix(mLocation, avatar->M);
		
		Graphics::setMatrix(vLocation, V);
		Graphics::setMatrix(pLocation, P);

		
		Graphics::end();
		Graphics::swapBuffers();
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
			default:
				break;
		}
	}
	
	void mouseMove(int windowId, int x, int y, int movementX, int movementY) {
		float rotationSpeed = 0.01;
		if (rotate) {
			globe.x() += (float)((mousePressX - x) * rotationSpeed);
			globe.z() += (float)((mousePressY - y) * rotationSpeed);
			mousePressX = x;
			mousePressY = y;
		}
	}
	
	void mousePress(int windowId, int button, int x, int y) {
		rotate = true;
		mousePressX = x;
		mousePressY = y;
	}
	
	void mouseRelease(int windowId, int button, int x, int y) {
		rotate = false;
	}
	
	void init() {
		FileReader vs("shader.vert");
		FileReader fs("shader.frag");
		vertexShader = new Shader(vs.readAll(), vs.size(), VertexShader);
		fragmentShader = new Shader(fs.readAll(), fs.size(), FragmentShader);
		
		// This defines the structure of your Vertex Buffer
		VertexStructure structure;
		structure.add("pos", Float3VertexData);
		structure.add("tex", Float2VertexData);
		structure.add("nor", Float3VertexData);
		
		program = new Program;
		program->setVertexShader(vertexShader);
		program->setFragmentShader(fragmentShader);
		program->link(structure);
		
		tex = program->getTextureUnit("tex");
		
		pLocation = program->getConstantLocation("P");
		vLocation = program->getConstantLocation("V");
		mLocation = program->getConstantLocation("M");
		
		cube = new MeshObject("cube.ogex", "", structure);
        cube->M = mat4::Translation(5, 0, 0);
        avatar = new MeshObject("avatar/avatar.ogex", "avatar/", structure);
        avatar->M = mat4::Translation(-5, 0, 0);
		
		Graphics::setRenderState(DepthTest, true);
		Graphics::setRenderState(DepthTestCompare, ZCompareLess);
		
		Graphics::setTextureAddressing(tex, Kore::U, Repeat);
		Graphics::setTextureAddressing(tex, Kore::V, Repeat);
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
