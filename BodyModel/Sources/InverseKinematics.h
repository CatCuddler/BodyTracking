#pragma once

#include <Kore/Graphics2/Graphics.h>

struct BoneNode;

using namespace Kore;

class InverseKinematics {
	
public:
	InverseKinematics(std::vector<BoneNode*> bones);
	bool inverseKinematics(Kore::vec4 desiredPos, BoneNode* targetBone);
	
private:
	int boneCount;
	int maxSteps;
	float maxError;
	int lastIndex = 3;
	
	std::vector<BoneNode*> bones;
	
	static const int maxBones = 10;
	typedef Matrix<6, maxBones, float> mat6x;
	typedef Matrix<6, 1, float> mat6x1;
	
	mat6x calcJacobian(BoneNode* targetBone, Kore::vec4 angle);
	mat6x getPsevdoInverse(mat6x jacobian);
	
	void applyChanges(std::vector<float> theta, BoneNode* targetBone);
	void updateBonePosition(BoneNode* targetBone);
	
	Kore::vec3 getAngles(Kore::mat4 rotationMat);
	
	float getRadians(float degree);

};
