#pragma once

#include <Kore/Graphics2/Graphics.h>

struct BoneNode;

using namespace Kore;

class InverseKinematics {
	
public:
	static const int num = 2;
	typedef Matrix<6, num, float> mat6x;
	
	InverseKinematics();
	bool inverseKinematics(Kore::vec4 desiredPos, BoneNode* targetBone);
	
private:
	int maxSteps;
	float maxError;
	
	const int parentCount = 53;
	
	mat6x calcJacobian(BoneNode* targetBone);
	mat6x getPsevdoInverse(mat6x jacobian);

};
