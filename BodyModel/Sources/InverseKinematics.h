#pragma once

#include <Kore/Math/Quaternion.h>

#include <vector>

struct BoneNode;

class InverseKinematics {
	
public:
	InverseKinematics(std::vector<BoneNode*> bones, int maxSteps);
    bool inverseKinematics(BoneNode* targetBone, Kore::vec4 desiredPosition, Kore::Quaternion desiredRotation, int jointDOFs, bool posAndOrientation = true);
	float getAverageIter();
	
private:
	int boneCount;
	int maxSteps;
	float maxError;
	int rootIndex;
	std::vector<BoneNode*> bones;
    
    static const int ikMode = 4; // 0: JT, 1: JPI, 2: DLS, 3: SVD, 4: DLS with SVD, 5: SDLS
	
	void setJointConstraints();
	void applyChanges(std::vector<float> deltaTheta, BoneNode* targetBone);
	void updateBonePosition(BoneNode* targetBone);
	void applyJointConstraints(BoneNode* targetBone);
	bool clampValue(float minVal, float maxVal, float* value);
	
	int totalNum;
	int sumIter;
};
