#include "pch.h"

#include "RotationUtility.h"

#include <math.h>

void Kore::RotationUtility::eulerToQuat(const float roll, const float pitch, const float yaw, Kore::Quaternion* quat) {
	float cr, cp, cy, sr, sp, sy, cpcy, spsy;
	// calculate trig identities
	cr = Kore::cos(roll / 2.0f);
	cp = Kore::cos(pitch / 2.0f);
	cy = Kore::cos(yaw / 2.0f);
	sr = Kore::sin(roll / 2.0f);
	sp = Kore::sin(pitch / 2.0f);
	sy = Kore::sin(yaw / 2.0f);
	cpcy = cp * cy;
	spsy = sp * sy;
	quat->w = cr * cpcy + sr * spsy;
	quat->x = sr * cpcy - cr * spsy;
	quat->y = cr * sp * cy + sr * cp * sy;
	quat->z = cr * cp * sy - sr * sp * cy;
}

void Kore::RotationUtility::quatToEuler(const Kore::Quaternion* quat, float* roll, float* pitch, float* yaw) {
	float ysqr = quat->y * quat->y;
	
	// roll (x-axis rotation)
	float t0 = 2.0 * (quat->w * quat->x + quat->y * quat->z);
	float t1 = 1.0 - 2.0 * (quat->x * quat->x + ysqr);
	*roll = Kore::atan2(t0, t1);
	
	// pitch (y-axis rotation)
	float t2 = 2.0 * (quat->w * quat->y - quat->z * quat->x);
	t2 = t2 > 1.0 ? 1.0 : t2;
	t2 = t2 < -1.0 ? -1.0 : t2;
	*pitch = Kore::asin(t2);
	
	// yaw (z-axis rotation)
	float t3 = 2.0 * (quat->w * quat->z + quat->x * quat->y);
	float t4 = 1.0 - 2.0 * (ysqr + quat->z * quat->z);
	*yaw = Kore::atan2(t3, t4);
}

float Kore::RotationUtility::getRadians(float degree) {
	const double halfC = Kore::pi / 180.0f;
	return degree * halfC;
}

float Kore::RotationUtility::getDegree(float rad) {
	const double halfC = 180.0f / Kore::pi;
	return rad * halfC;
}

void Kore::RotationUtility::getOrientation(const Kore::mat4* m, Kore::Quaternion* orientation) {
	orientation->w = Kore::sqrt(Kore::max(0.0, 1.0 + m->get(0, 0) + m->get(1, 1) + m->get(2, 2))) / 2.0;
	orientation->x = Kore::sqrt(Kore::max(0.0, 1.0 + m->get(0, 0) - m->get(1, 1) - m->get(2, 2))) / 2.0;
	orientation->y = Kore::sqrt(Kore::max(0.0, 1.0 - m->get(0, 0) + m->get(1, 1) - m->get(2, 2))) / 2.0;
	orientation->z = Kore::sqrt(Kore::max(0.0, 1.0 - m->get(0, 0) - m->get(1, 1) + m->get(2, 2))) / 2.0;
	orientation->x = copysign(orientation->x, m->get(2, 1) - m->get(1, 2));
	orientation->y = copysign(orientation->y, m->get(0, 2) - m->get(2, 0));
	orientation->z = copysign(orientation->z, m->get(1, 0) - m->get(0, 1));
	orientation->normalize();
}


