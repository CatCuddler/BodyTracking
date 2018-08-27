#pragma once

#include <Kore/Math/Quaternion.h>

namespace Kore {
	
	namespace RotationUtility {
		void eulerToQuat(const float roll, const float pitch, const float yaw, Kore::Quaternion* quat);
		void quatToEuler(const Kore::Quaternion* quat, float* roll, float* pitch, float* yaw);
		float getRadians(float degree);
		float getDegree(float rad);
		void getOrientation(const Kore::mat4* m, Kore::Quaternion* orientation);
		Kore::Quaternion matrixToQuaternion(Kore::mat4 diffRot, int i = 0);
	}
}
