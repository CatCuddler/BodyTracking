#pragma once

#include <Kore/Math/Quaternion.h>

namespace Kore {

	namespace RotationUtility {
		void eulerToQuat(float roll, float pitch, float yaw, Kore::Quaternion* quat);
		float getRadians(float degree);
	}
}
