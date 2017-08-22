#pragma once

#include <Kore/Graphics4/Graphics.h>
#include <Kore/Math/Quaternion.h>

namespace Logger {
	void saveData(Kore::vec3 rawPos, Kore::Quaternion rawRot);
	void saveInitTransAndRot(Kore::mat4 initTrans, Kore::Quaternion initRot);
	bool readData(int line, const char* filename, Kore::vec3 *rawPos, Kore::Quaternion *rawRot);
}
