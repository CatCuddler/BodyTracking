#pragma once

#include <Kore/Graphics2/Graphics.h>

struct BoneNode;

using namespace Kore;

class InverseKinematics {
	
public:
	InverseKinematics();
	bool inverseKinematics(Kore::vec4 desiredPos, BoneNode* targetBone);
	
private:
	int boneCount;
	int maxSteps;
	float maxError;
	
	typedef Matrix<6, 54, float> mat6x;
	
	mat6x calcJacobian(BoneNode* targetBone);
	mat6x getPsevdoInverse(mat6x jacobian);
	
	Kore::vec3 getAngles(Kore::mat4 rotationMat);

};
