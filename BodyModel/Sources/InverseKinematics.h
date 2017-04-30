#pragma once

#include <Kore/Graphics2/Graphics.h>
#include "MatrixX.h"

struct BoneNode;

using namespace Kore;

class InverseKinematics {
	
public:
	InverseKinematics();
	bool inverseKinematics(Kore::vec4 desiredPos, BoneNode* targetBone);
	
private:
	int maxSteps;
	float maxError;
	
	typedef Matrix<6, 54, float> mat6x;
	typedef Matrix<54, 6, float> mat54x;
	typedef Matrix<1, 6, float> mat6x1;

	
	mat6x calcJacobian(BoneNode* targetBone);
	mat54x getPsevdoInverse(mat6x jacobian);
	
	void applyChanges(MatrixX theta, BoneNode* targetBone);
	
	Kore::vec3 getAngles(Kore::mat4 rotationMat);

};
