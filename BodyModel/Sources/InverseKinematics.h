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
	int rootIndex;
	
	std::vector<BoneNode*> bones;
	
	static const int maxBones = 10;
	typedef Matrix<3, maxBones, float> mat3x;
	typedef Matrix<3, 1, float> mat3x1;
	
	bool checkBoneIndex(int boneIndex);
	
	mat3x calcJacobian(BoneNode* targetBone, Kore::vec4 angle);
	Kore::mat4 relPose(int i, int j);
	mat3x getPsevdoInverse(mat3x jacobian);
	
	void applyChanges(std::vector<float> theta, BoneNode* targetBone);
	void updateBonePosition(BoneNode* targetBone);
	
	Kore::mat4 quaternionToMatrix(Kore::vec4 quat);
	Kore::vec3 getAngles(Kore::mat4 rotationMat);
	
	float getRadians(float degree);

};
