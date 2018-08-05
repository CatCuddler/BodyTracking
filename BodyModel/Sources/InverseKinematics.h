#pragma once

#include <Kore/Math/Quaternion.h>
#include <float.h>
#include "Jacobian.h"

class InverseKinematics {
	
public:
	InverseKinematics(std::vector<BoneNode*> bones);
	void inverseKinematics(BoneNode* targetBone, Kore::vec3 desPosition, Kore::Quaternion desRotation);
	void initializeBone(BoneNode* bone);
	
	float getReached();
	float* getIterations();
	float* getErrorPos();
	float* getErrorRot();
	float* getTime();
	float* getTimeIteration();
	
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
	
	int totalNum = 0, evalReached = 0;
	float evalIterations[3], evalErrorPos[3], evalErrorRot[3], evalTime[3], evalTimeIteration[3];
};
