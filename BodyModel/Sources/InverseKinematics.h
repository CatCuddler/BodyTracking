#pragma once

#include <Kore/Math/Quaternion.h>
#include "Jacobian.h"

#include <vector>

struct BoneNode;

class InverseKinematics {
	
public:
	InverseKinematics(std::vector<BoneNode*> bones, int maxSteps);
    bool inverseKinematics(BoneNode* targetBone, Kore::vec4 desiredPosition, Kore::Quaternion desiredRotation);
	float getAverageIter();
	
private:
	int boneCount;
	int maxSteps;
	float maxError;
	int rootIndex;
	std::vector<BoneNode*> bones;
	
	void setJointConstraints();
	void applyChanges(Jacobian::vec_n deltaTheta, BoneNode* targetBone);
	void updateBonePosition(BoneNode* targetBone);
	void applyJointConstraints(BoneNode* targetBone);
	bool clampValue(float minVal, float maxVal, float* value);
	
	int totalNum;
	int sumIter;
};
