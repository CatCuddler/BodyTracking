#pragma once

#include <Kore/Math/Quaternion.h>
#include <float.h>
#include "Jacobian.h"

struct BoneNode;

class InverseKinematics {
	
public:
	InverseKinematics(std::vector<BoneNode*> bones);
	bool inverseKinematics(BoneNode* targetBone, Kore::vec4 desiredPosition, Kore::Quaternion desiredRotation);
	int getTotalNum();
	float getAverageIter();
	float getMinIter();
	float getAverageReached();
	float getAverageError();
	float getMinError();
	float getMaxError();
	
private:
	int boneCount;
	int rootIndex = 2;
	std::vector<BoneNode*> bones;
	
	// 0: JT, 1: JPI, 2: DLS, 3: SVD, 4: DLS with SVD, 5: SDLS
	static const int backIkMode = 5;
	static const int backJointDOFs = 3;
	static const bool backWithOrientation = true;
	static const int handIkMode = 5;
	static const int handJointDOFs = 6; // 4 without hands
	static const bool handWithOrientation = true;
	static const int footIkMode = 0;
	static const int footJointDOFs = 4;
	static const bool footWithOrientation = false;
	
	int maxSteps = 100;
	float errorMax = 0.01f;
	
	Jacobian<backJointDOFs, backWithOrientation>* jacobianBack = new Jacobian<backJointDOFs, backWithOrientation>;
	Jacobian<handJointDOFs, handWithOrientation>* jacobianHand = new Jacobian<handJointDOFs, handWithOrientation>;
	Jacobian<footJointDOFs, footWithOrientation>* jacobianFoot = new Jacobian<footJointDOFs, footWithOrientation>;
	
	void setJointConstraints();
	void applyChanges(std::vector<float> deltaTheta, BoneNode* targetBone);
	void updateBonePosition(BoneNode* targetBone);
	void applyJointConstraints(BoneNode* targetBone);
	bool clampValue(float minVal, float maxVal, float* value);
	
	int totalNum = 0, sumIter = 0, sumReached = 0;
	float sumError = 0, minError = FLT_MAX, maxError = 0;
};
