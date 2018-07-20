#pragma once

#include <Kore/Math/Quaternion.h>
#include <float.h>
#include "Jacobian.h"

class InverseKinematics {
	
public:
	InverseKinematics(std::vector<BoneNode*> bones);
	bool inverseKinematics(BoneNode* targetBone, Kore::vec3 desPosition, Kore::Quaternion desRotation);
	int getTotalNum();
	float getAverageIter();
	float getMinIter();
	float getAverageReached();
	float getAverageError();
	float getMinError();
	float getMaxError();
	
private:
	std::vector<BoneNode*> bones;
	
	static const int handJointDOFs = 6;
	static const bool handWithOrientation = true;
	Jacobian<handJointDOFs, handWithOrientation>* jacobianHand = new Jacobian<handJointDOFs, handWithOrientation>;
	
	static const int footJointDOFs = 4;
	static const bool footWithOrientation = false;
	Jacobian<footJointDOFs, footWithOrientation>* jacobianFoot = new Jacobian<footJointDOFs, footWithOrientation>;
	
	int maxSteps = 100;
	float errorMax = 0.01f;
	
	void setJointConstraints();
	void applyChanges(std::vector<float> deltaTheta, BoneNode* targetBone);
	void applyJointConstraints(BoneNode* targetBone);
	bool clampValue(float minVal, float maxVal, float* value);
	
	int totalNum = 0, sumIter = 0, sumReached = 0;
	float sumError = 0, minError = FLT_MAX, maxError = 0;
};
