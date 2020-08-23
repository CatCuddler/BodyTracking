#include "pch.h"
#include "InverseKinematics.h"
#include "RotationUtility.h"

InverseKinematics::InverseKinematics(std::vector<BoneNode*> boneVec) {
	bones = boneVec;
	setJointConstraints();
}

void InverseKinematics::inverseKinematics(BoneNode* targetBone, IKMode ikMode, Kore::vec3 desPosition, Kore::Quaternion desRotation) {
	std::vector<float> deltaTheta, prevDeltaTheta;
	float errorPos = -1.0f;
	float errorRot = -1.0f;
	bool stucked = false;
	
	int i = 0;
	// while position not reached and maxStep not reached and not stucked
	while ((errorPos < 0 || errorPos > errorMaxPos[ikMode] || errorRot < 0 || errorRot > errorMaxRot[ikMode]) && i < (int) maxSteps[ikMode] && !stucked) {
		
		prevDeltaTheta = deltaTheta;
		
		// todo: better!
		if (targetBone->nodeIndex == leftForeArmBoneIndex || targetBone->nodeIndex == rightForeArmBoneIndex) {
			// deltaTheta = jacobianHand->calcDeltaTheta(targetBone, desPosition, desRotation, tracker[1]->ikMode);
			deltaTheta = jacobianHand->calcDeltaTheta(targetBone, desPosition, desRotation, ikMode); // todo: remove after eval
			errorPos = jacobianHand->getPositionError();
			errorRot = jacobianHand->getRotationError();
		} else if (targetBone->nodeIndex == leftFootBoneIndex|| targetBone->nodeIndex == rightFootBoneIndex) {
			// deltaTheta = jacobianFoot->calcDeltaTheta(targetBone, desPosition, desRotation, tracker[3]->ikMode);
			deltaTheta = jacobianFoot->calcDeltaTheta(targetBone, desPosition, desRotation, ikMode); // todo: remove after eval
			errorPos = jacobianFoot->getPositionError();
			errorRot = jacobianFoot->getRotationError();
		} else if (targetBone->nodeIndex == headBoneIndex) {
			deltaTheta = jacobianHead->calcDeltaTheta(targetBone, desPosition, desRotation, ikMode); // todo: remove after eval
			errorPos = jacobianHead->getPositionError();
			errorRot = jacobianHead->getRotationError();
		}
		
		// check if ik stucked (runned in extrema)
		if (i) {
			float sum = 0;
			int j = 0;
			while (!stucked && j < prevDeltaTheta.size()) {
				sum += fabs(prevDeltaTheta[j] - deltaTheta[j]);
				j++;
			}
			stucked = sum < nearNull;
		}
		
		applyChanges(deltaTheta, targetBone);
		applyJointConstraints(targetBone);
		for (int i = 0; i < bones.size(); ++i)
			updateBone(bones[i]);
		
		i++;
	}
}

void InverseKinematics::updateBone(BoneNode* bone) {
	if (bone->parent->initialized)
		bone->combined = bone->parent->combined * bone->local;
}

void InverseKinematics::initializeBone(BoneNode* bone) {
	updateBone(bone);
	
	if (!bone->initialized) {
		bone->initialized = true;
		bone->combinedInv = bone->combined.Invert();
	}
	
	bone->finalTransform = bone->combined * bone->combinedInv;
}

void InverseKinematics::applyChanges(std::vector<float> deltaTheta, BoneNode* targetBone) {
	unsigned long size = deltaTheta.size();
	int i = 0;
	
	BoneNode* bone = targetBone;
	while (bone->initialized && i < size) {
		Kore::vec3 axes = bone->axes;
		
		if (axes.x() == 1.0 && i < size) bone->rotation.rotate(Kore::Quaternion(Kore::vec3(1, 0, 0), deltaTheta[i++]));
		if (axes.y() == 1.0 && i < size) bone->rotation.rotate(Kore::Quaternion(Kore::vec3(0, 1, 0), deltaTheta[i++]));
		if (axes.z() == 1.0 && i < size) bone->rotation.rotate(Kore::Quaternion(Kore::vec3(0, 0, 1), deltaTheta[i++]));
		
		bone->rotation.normalize();
		bone->local = bone->transform * bone->rotation.matrix().Transpose();
		
		bone = bone->parent;
	}
}

void InverseKinematics::applyJointConstraints(BoneNode* targetBone) {
	BoneNode* bone = targetBone;
	while (bone->initialized) {
		Kore::vec3 axes = bone->axes;
		
		Kore::vec3 rot;
		Kore::RotationUtility::quatToEuler(&bone->rotation, &rot.x(), &rot.y(), &rot.z());
		
		float x = rot.x(), y = rot.y(), z = rot.z();
		if (axes.x() == 1.0) {
			clampValue(bone->constrain[xMin], bone->constrain[xMax], x);
		}
		if (axes.y() == 1.0) {
			clampValue(bone->constrain[yMin], bone->constrain[yMax], y);
		}
		if (axes.z() == 1.0) {
			clampValue(bone->constrain[zMin], bone->constrain[zMax], z);
		}
		Kore::RotationUtility::eulerToQuat(x, y, z, &bone->rotation);
		
		// bone->rotation = Kore::Quaternion((double) x, (double) y, (double) z, 1);
		bone->rotation.normalize();
		bone->local = bone->transform * bone->rotation.matrix().Transpose();
		bone = bone->parent;
	}
}

void InverseKinematics::clampValue(float minVal, float maxVal, float& value) {
	if (minVal > maxVal) {
		float temp = minVal;
		minVal = maxVal;
		maxVal = temp;
	}
	
	if (value < minVal)
		value = minVal;
	else if (value > maxVal)
		value = maxVal;
}

float InverseKinematics::getRadian(float degree) {
	return degree * (Kore::pi / 180.0f);
}

void InverseKinematics::setJointConstraints() {
	BoneNode* nodeLeft;
	BoneNode* nodeRight;
	
	float tolerance = getRadian(15);
	
	// Neck
	nodeLeft = bones[neckBoneIndex - 1];
	nodeLeft->axes = Kore::vec3(1, 1, 1);
	nodeLeft->constrain[xMin] = -getRadian(45) - tolerance;		nodeLeft->constrain[xMax] = getRadian(45) + tolerance;
	nodeLeft->constrain[yMin] = -getRadian(45) - tolerance;		nodeLeft->constrain[yMax] = getRadian(45) + tolerance;
	nodeLeft->constrain[zMin] = -getRadian(45) - tolerance;		nodeLeft->constrain[zMax] = getRadian(45) + tolerance;
	
	// Upper body
/*	nodeLeft = bones[upperBack - 1];
	nodeLeft->axes = Kore::vec3(1, 1, 1);
	nodeLeft->constrain[xMin] = -getRadian(30) - tolerance;		nodeLeft->constrain[xMax] = getRadian(30) + tolerance;
	nodeLeft->constrain[yMin] = -getRadian(20) - tolerance;		nodeLeft->constrain[yMax] = getRadian(20) + tolerance;
	nodeLeft->constrain[zMin] = -getRadian(20) - tolerance;		nodeLeft->constrain[zMax] = getRadian(20) + tolerance;*/
	
	// Upperarm
	nodeLeft = bones[leftArmBoneIndex - 1];
	nodeLeft->axes = Kore::vec3(1, 1, 1);
	nodeLeft->constrain[xMin] = -getRadian(50) - tolerance;		nodeLeft->constrain[xMax] = getRadian(180) + tolerance;
	nodeLeft->constrain[yMin] = -getRadian(90) - tolerance;		nodeLeft->constrain[yMax] = getRadian(90) + tolerance;
	nodeLeft->constrain[zMin] = -getRadian(90) - tolerance;		nodeLeft->constrain[zMax] = getRadian(90) + tolerance;
	
	nodeRight = bones[rightArmBoneIndex - 1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain[xMin] = nodeLeft->constrain[xMin];		nodeRight->constrain[xMax] = nodeLeft->constrain[xMax];
	nodeRight->constrain[yMin] = -nodeLeft->constrain[yMin],	nodeRight->constrain[yMax] = -nodeLeft->constrain[yMax];
	nodeRight->constrain[zMin] = -nodeLeft->constrain[zMin],	nodeRight->constrain[zMax] = -nodeLeft->constrain[zMax];
	
	// Forearm
	nodeLeft = bones[leftForeArmBoneIndex - 1];
	nodeLeft->axes = Kore::vec3(1, 0, 0);
	nodeLeft->constrain[xMin] = -getRadian(10) - tolerance;		nodeLeft->constrain[xMax] = getRadian(140) + tolerance;
	
	nodeRight = bones[rightForeArmBoneIndex - 1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain[xMin] = nodeLeft->constrain[xMin];		nodeRight->constrain[xMax] = nodeLeft->constrain[xMax];
	
	// Hand
	nodeLeft = bones[leftHandBoneIndex - 1];
	nodeLeft->axes = Kore::vec3(1, 0, 1);
	nodeLeft->constrain[xMin] = -getRadian(20) - tolerance;		nodeLeft->constrain[xMax] = getRadian(30) + tolerance;
	nodeLeft->constrain[zMin] = -getRadian(60) - tolerance;		nodeLeft->constrain[zMax] = getRadian(60) + tolerance;
	
	nodeRight = bones[rightHandBoneIndex - 1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain[xMin] = nodeLeft->constrain[xMin];		nodeRight->constrain[xMax] = nodeLeft->constrain[xMax];
	nodeRight->constrain[zMin] = -nodeLeft->constrain[zMin],	nodeRight->constrain[zMax] = -nodeLeft->constrain[zMax];
	
	// Thigh
	nodeLeft = bones[leftUpLegBoneIndex - 1];
	nodeLeft->axes = Kore::vec3(1, 1, 1);
	nodeLeft->constrain[xMin] = -getRadian(110) - tolerance;	nodeLeft->constrain[xMax] = getRadian(30) + tolerance;
	nodeLeft->constrain[yMin] = -getRadian(60) - tolerance;		nodeLeft->constrain[yMax] = getRadian(40) + tolerance;
	nodeLeft->constrain[zMin] = -getRadian(50) - tolerance;		nodeLeft->constrain[zMax] = getRadian(20) + tolerance;
	
	nodeRight = bones[rightUpLegBoneIndex - 1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain[xMin] = nodeLeft->constrain[xMin];		nodeRight->constrain[xMax] = nodeLeft->constrain[xMax];
	nodeRight->constrain[yMin] = -nodeLeft->constrain[yMin],	nodeRight->constrain[yMax] = -nodeLeft->constrain[yMax];
	nodeRight->constrain[zMin] = -nodeLeft->constrain[zMin],	nodeRight->constrain[zMax] = -nodeLeft->constrain[zMax];
	
	// Calf
	nodeLeft = bones[leftLegBoneIndex - 1];
	nodeLeft->axes = Kore::vec3(1, 0, 0);
	nodeLeft->constrain[xMin] = -getRadian(10) - tolerance;		nodeLeft->constrain[xMax] = getRadian(140) + tolerance;
	
	nodeRight = bones[rightLegBoneIndex - 1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain[xMin] = nodeLeft->constrain[xMin];		nodeRight->constrain[xMax] = nodeLeft->constrain[xMax];
}
