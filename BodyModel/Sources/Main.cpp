#include "pch.h"

#include <Kore/Graphics/Graphics.h>
#include <Kore/Input/Keyboard.h>
#include <Kore/System.h>
#include <Kore/Log.h>

#include "MeshObject.h"

using namespace Kore;

namespace {
	
	bool left;
	bool right;
	bool down;
	bool up;
	
	MeshObject* cube;
	
	void update() {
		Graphics::begin();
		
		Graphics::end();
		Graphics::swapBuffers();
	}
	
	void keyDown(KeyCode code, wchar_t character) {
		switch (code) {
			case Kore::Key_Left:
				left = true;
				break;
			case Kore::Key_Right:
				right = true;
				break;
			case Kore::Key_Down:
				down = true;
				break;
			case Kore::Key_Up:
				up = true;
				break;
			default:
				break;
		}
	}
	
	void keyUp(KeyCode code, wchar_t character) {
		switch (code) {
			case Kore::Key_Left:
				left = false;
				break;
			case Kore::Key_Right:
				right = false;
				break;
			case Kore::Key_Down:
				down = false;
				break;
			case Kore::Key_Up:
				up = false;
				break;
			default:
				break;
		}
	}
}

int kore(int argc, char** argv) {
	int w = 1024;
	int h = 512;
	
	System::init("BodyTracking", w, h);
	
	System::setCallback(update);
	
	Keyboard::the()->KeyDown = keyDown;
	Keyboard::the()->KeyUp = keyUp;
	
	cube = new MeshObject("cube.ogex");
	
	System::start();
	
	System::stop();
	
	return 0;
}
