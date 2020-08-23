#pragma once

#include "Jacobian.h"

#include <Kore/Math/Quaternion.h>

class InverseKinematics {
	
public:
	InverseKinematics(std::vector<BoneNode*> bones);
	void inverseKinematics(BoneNode* targetBone, IKMode ikMode, Kore::vec3 desPosition, Kore::Quaternion desRotation);
	void initializeBone(BoneNode* bone);
	
private:
	std::vector<BoneNode*> bones;
	
	static const int handJointDOFs = 6;
	Jacobian<handJointDOFs>* jacobianHand = new Jacobian<handJointDOFs>;
	
	static const int footJointDOFs = 4;
	Jacobian<footJointDOFs>* jacobianFoot = new Jacobian<footJointDOFs>;
	
	static const int headJointDOFs = 3;
	Jacobian<headJointDOFs>* jacobianHead = new Jacobian<headJointDOFs>;

	
	float getRadian(float degree);
	
	void updateBone(BoneNode* bone);
	
	const char* const xMin = "x_min";
	const char* const xMax = "x_max";
	const char* const yMin = "y_min";
	const char* const yMax = "y_max";
	const char* const zMin = "z_min";
	const char* const zMax = "z_max";
	
	void setJointConstraints();
	void applyChanges(std::vector<float> deltaTheta, BoneNode* targetBone);
	void applyJointConstraints(BoneNode* targetBone);
	void clampValue(float minVal, float maxVal, float& value);
};
