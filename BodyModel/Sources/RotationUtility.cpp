#include "pch.h"

#include "RotationUtility.h"

void Kore::RotationUtility::eulerToQuat(float roll, float pitch, float yaw, Kore::Quaternion* quat) {
	float cr, cp, cy, sr, sp, sy, cpcy, spsy;
	// calculate trig identities
	cr = Kore::cos(roll/2);
	cp = Kore::cos(pitch/2);
	cy = Kore::cos(yaw/2);
	sr = Kore::sin(roll/2);
	sp = Kore::sin(pitch/2);
	sy = Kore::sin(yaw/2);
	cpcy = cp * cy;
	spsy = sp * sy;
	quat->w = cr * cpcy + sr * spsy;
	quat->x = sr * cpcy - cr * spsy;
	quat->y = cr * sp * cy + sr * cp * sy;
	quat->z = cr * cp * sy - sr * sp * cy;
}

float Kore::RotationUtility::getRadians(float degree) {
	const double halfC = Kore::pi / 180.0f;
	return degree * halfC;
}


