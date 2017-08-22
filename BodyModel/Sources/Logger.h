#pragma once

#include <Kore/Graphics4/Graphics.h>
#include <Kore/Math/Quaternion.h>

namespace Logger {
	void saveData(Kore::vec3 rawPos, Kore::Quaternion rawRot);
	void readData(int line, Kore::vec3 &rawPos, Kore::Quaternion &rawRot);
}
