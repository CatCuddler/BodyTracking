#pragma once

#include <Kore/Math/Quaternion.h>
#include <float.h>
#include "Jacobian.h"

class InverseKinematics {
	
public:
	InverseKinematics(std::vector<BoneNode*> bones);
	bool inverseKinematics(BoneNode* targetBone, Kore::vec3 desPosition, Kore::Quaternion desRotation);
	void initializeBone(BoneNode* bone);
	
	float getAverageIter();
	float getMinIter();
	float getAverageReached();
	float getAverageTime();
	float getAverageTimeIteration();
	float getAverageError();
	float getAverageErrorPos();
	float getAverageErrorRot();
	float getMinError();
	float getMaxError();
	
	void resetStats();
	
private:
	std::vector<BoneNode*> bones;
	
	static const int handJointDOFs = 6;
	static const bool handWithOrientation = ikMode == 0 ? false : withOrientation;
	Jacobian<handJointDOFs, handWithOrientation>* jacobianHand = new Jacobian<handJointDOFs, handWithOrientation>;
	
	static const int footJointDOFs = 4;
	static const bool footWithOrientation = ikMode == 0 ? false : withOrientation;
	Jacobian<footJointDOFs, footWithOrientation>* jacobianFoot = new Jacobian<footJointDOFs, footWithOrientation>;
	
	void updateBone(BoneNode* bone);
	void setJointConstraints();
	void applyChanges(std::vector<float> deltaTheta, BoneNode* targetBone);
	void applyJointConstraints(BoneNode* targetBone);
	float clampValue(float minVal, float maxVal, float value);
	
	int totalNum = 0, sumIter = 0, sumReached = 0;
	float sumError = 0, sumErrorPos = 0, sumErrorRot = 0, sumTime = 0, sumTimeIteration = 0, minError = FLT_MAX, maxError = 0;
};
