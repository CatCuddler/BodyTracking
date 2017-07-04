#pragma once

#include <Kore/Graphics2/Graphics.h>
#include <Kore/Math/Quaternion.h>

struct BoneNode;

using namespace Kore;

class InverseKinematics {
	
public:
	InverseKinematics(std::vector<BoneNode*> bones, int maxSteps);
	bool inverseKinematics(BoneNode* targetBone, Kore::vec4 desiredPosition, Kore::vec3 desiredRotation);
	
private:
	int boneCount;
	int maxSteps;
	float maxError;
	int rootIndex;
	
	std::vector<BoneNode*> bones;
	
	static const int jacDim = 6;
	static const int maxBones = 8;
	typedef Matrix<jacDim, maxBones, float> mat6x;
	typedef Vector<float, jacDim> vec6;
	typedef Vector<float, maxBones> vec8;
	typedef Matrix<jacDim, jacDim, float> mat6x6;
	typedef Matrix<maxBones, maxBones, float> mat8x8;
	
	void setJointConstraints();
	
	mat6x calcJacobian(BoneNode* targetBone, Kore::vec4 rotAxis);
	mat6x getPsevdoInverse(mat6x jacobian);
	
	void applyChanges(std::vector<float> theta, BoneNode* targetBone);
	void updateBonePosition(BoneNode* targetBone);

	void clampValue(float minVal, float maxVal, float* value);
};
