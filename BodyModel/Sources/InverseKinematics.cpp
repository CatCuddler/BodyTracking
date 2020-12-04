#include "pch.h"
#include "InverseKinematics.h"
#include "RotationUtility.h"

#include <Kore/Log.h>

using namespace Kore;

InverseKinematics::InverseKinematics(std::vector<BoneNode*> boneVec) {
	bones = boneVec;
	setJointConstraints();
}

void InverseKinematics::inverseKinematics(BoneNode* targetBone, IKMode ikMode, Kore::vec3 desPosition, Kore::Quaternion desRotation) {
	std::vector<float> deltaTheta;
	float previousPosition;
	float previousRotation;
	float errorPos = maxfloat();
	float errorRot = maxfloat();
	bool stucked = false;
	
	int i = 0;
	// while position not reached and maxStep not reached and not stucked
	while ((errorPos > errorMaxPos[ikMode] || errorRot > errorMaxRot[ikMode]) && i < (int) maxIterations[ikMode] && !stucked) {
		
		previousPosition = errorPos;
		previousRotation = errorRot;
		
		if (simpleIK && (targetBone->nodeIndex == leftHandBoneIndex || targetBone->nodeIndex == rightHandBoneIndex)) {
			deltaTheta = jacobianSimpleIKHand->calcDeltaTheta(targetBone, desPosition, desRotation, ikMode);
			errorPos = jacobianSimpleIKHand->getPositionError();
			errorRot = jacobianSimpleIKHand->getRotationError();
		} else if (!simpleIK && (targetBone->nodeIndex == leftForeArmBoneIndex || targetBone->nodeIndex == rightForeArmBoneIndex)) {
			deltaTheta = jacobianHand->calcDeltaTheta(targetBone, desPosition, desRotation, ikMode);
			errorPos = jacobianHand->getPositionError();
			errorRot = jacobianHand->getRotationError();
		} else if (targetBone->nodeIndex == leftFootBoneIndex|| targetBone->nodeIndex == rightFootBoneIndex) {
			deltaTheta = jacobianFoot->calcDeltaTheta(targetBone, desPosition, desRotation, ikMode);
			errorPos = jacobianFoot->getPositionError();
			errorRot = jacobianFoot->getRotationError();
		} else if (targetBone->nodeIndex == headBoneIndex) {
			deltaTheta = jacobianHead->calcDeltaTheta(targetBone, desPosition, desRotation, ikMode);
			errorPos = jacobianHead->getPositionError();
			errorRot = jacobianHead->getRotationError();
		}
		
		// check if ik stucked (runned in extrema)
		if (i) {
			if (fabs(previousPosition - errorPos) < nearNull) {
				stucked = true;
			}
			if (fabs(previousRotation - errorRot) < nearNull) {
				stucked = true;
			}
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
	
	float tolerance = RotationUtility::getRadians(15);
	
	// Head
	nodeLeft = bones[headBoneIndex - 1];
	nodeLeft->axes = Kore::vec3(1, 1, 1);
	nodeLeft->constrain[xMin] = -RotationUtility::getRadians(40) - tolerance;		nodeLeft->constrain[xMax] = RotationUtility::getRadians(40) + tolerance;
	nodeLeft->constrain[yMin] = -RotationUtility::getRadians(90) - tolerance;		nodeLeft->constrain[yMax] = RotationUtility::getRadians(90) + tolerance;
	nodeLeft->constrain[zMin] = -RotationUtility::getRadians(60) - tolerance;		nodeLeft->constrain[zMax] = RotationUtility::getRadians(60) + tolerance;
	
	log(LogLevel::Info, "Head");
	log(LogLevel::Info, "xmin %f xmax %f", RotationUtility::getDegree(nodeLeft->constrain[xMin]), RotationUtility::getDegree(nodeLeft->constrain[xMax]));
	log(LogLevel::Info, "ymin %f ymax %f", RotationUtility::getDegree(nodeLeft->constrain[yMin]), RotationUtility::getDegree(nodeLeft->constrain[yMax]));
	log(LogLevel::Info, "zmin %f zmax %f", RotationUtility::getDegree(nodeLeft->constrain[zMin]), RotationUtility::getDegree(nodeLeft->constrain[zMax]));
	
	// Neck
	nodeLeft = bones[spineBoneIndex - 1];
	nodeLeft->axes = Kore::vec3(1, 0, 1);
	nodeLeft->constrain[xMin] = -RotationUtility::getRadians(20) - tolerance;		nodeLeft->constrain[xMax] = RotationUtility::getRadians(0) + tolerance;
	nodeLeft->constrain[zMin] = -RotationUtility::getRadians(35) - tolerance;		nodeLeft->constrain[zMax] = RotationUtility::getRadians(35) + tolerance;
	
	log(LogLevel::Info, "Spine");
	log(LogLevel::Info, "xmin %f xmax %f", RotationUtility::getDegree(nodeLeft->constrain[xMin]), RotationUtility::getDegree(nodeLeft->constrain[xMax]));
	log(LogLevel::Info, "zmin %f zmax %f", RotationUtility::getDegree(nodeLeft->constrain[zMin]), RotationUtility::getDegree(nodeLeft->constrain[zMax]));
	
	// Upperarm
	nodeLeft = bones[leftArmBoneIndex - 1];
	nodeLeft->axes = Kore::vec3(1, 1, 1);
	nodeLeft->constrain[xMin] = -RotationUtility::getRadians(50) - tolerance;		nodeLeft->constrain[xMax] = RotationUtility::getRadians(130) + tolerance;
	nodeLeft->constrain[yMin] = -RotationUtility::getRadians(90) - tolerance;		nodeLeft->constrain[yMax] = RotationUtility::getRadians(90) + tolerance;
	nodeLeft->constrain[zMin] = -RotationUtility::getRadians(90) - tolerance;		nodeLeft->constrain[zMax] = RotationUtility::getRadians(90) + tolerance;
	
	log(LogLevel::Info, "Shoulder Left");
	log(LogLevel::Info, "xmin %f xmax %f", RotationUtility::getDegree(nodeLeft->constrain[xMin]), RotationUtility::getDegree(nodeLeft->constrain[xMax]));
	log(LogLevel::Info, "ymin %f ymax %f", RotationUtility::getDegree(nodeLeft->constrain[yMin]), RotationUtility::getDegree(nodeLeft->constrain[yMax]));
	log(LogLevel::Info, "zmin %f zmax %f", RotationUtility::getDegree(nodeLeft->constrain[zMin]), RotationUtility::getDegree(nodeLeft->constrain[zMax]));
	
	nodeRight = bones[rightArmBoneIndex - 1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain[xMin] = nodeLeft->constrain[xMin];		nodeRight->constrain[xMax] = nodeLeft->constrain[xMax];
	nodeRight->constrain[yMin] = -nodeLeft->constrain[yMin],	nodeRight->constrain[yMax] = -nodeLeft->constrain[yMax];
	nodeRight->constrain[zMin] = -nodeLeft->constrain[zMin],	nodeRight->constrain[zMax] = -nodeLeft->constrain[zMax];
	
	log(LogLevel::Info, "Shoulder Right");
	log(LogLevel::Info, "xmin %f xmax %f", RotationUtility::getDegree(nodeRight->constrain[xMin]), RotationUtility::getDegree(nodeRight->constrain[xMax]));
	log(LogLevel::Info, "ymin %f ymax %f", RotationUtility::getDegree(nodeRight->constrain[yMin]), RotationUtility::getDegree(nodeRight->constrain[yMax]));
	log(LogLevel::Info, "zmin %f zmax %f", RotationUtility::getDegree(nodeRight->constrain[zMin]), RotationUtility::getDegree(nodeRight->constrain[zMax]));
	
	// Forearm
	nodeLeft = bones[leftForeArmBoneIndex - 1];
	nodeLeft->axes = Kore::vec3(1, 0, 0);
	nodeLeft->constrain[xMin] = 0;								nodeLeft->constrain[xMax] = RotationUtility::getRadians(140) + tolerance;
	
	log(LogLevel::Info, "Elbow Left");
	log(LogLevel::Info, "xmin %f xmax %f", RotationUtility::getDegree(nodeLeft->constrain[xMin]), RotationUtility::getDegree(nodeLeft->constrain[xMax]));
	
	nodeRight = bones[rightForeArmBoneIndex - 1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain[xMin] = nodeLeft->constrain[xMin];		nodeRight->constrain[xMax] = nodeLeft->constrain[xMax];
	
	log(LogLevel::Info, "Elbow Right");
	log(LogLevel::Info, "xmin %f xmax %f", RotationUtility::getDegree(nodeRight->constrain[xMin]), RotationUtility::getDegree(nodeRight->constrain[xMax]));
	
	// Hand
	nodeLeft = bones[leftHandBoneIndex - 1];
	nodeLeft->axes = Kore::vec3(1, 1, 1);
	nodeLeft->constrain[xMin] = -RotationUtility::getRadians(30) - tolerance;		nodeLeft->constrain[xMax] = RotationUtility::getRadians(30) + tolerance;
	nodeLeft->constrain[yMin] = -RotationUtility::getRadians(80) - tolerance;		nodeLeft->constrain[yMax] = RotationUtility::getRadians(80) + tolerance;
	nodeLeft->constrain[zMin] = -RotationUtility::getRadians(60) - tolerance;		nodeLeft->constrain[zMax] = RotationUtility::getRadians(60) + tolerance;
	
	log(LogLevel::Info, "Hand Left");
	log(LogLevel::Info, "xmin %f xmax %f", RotationUtility::getDegree(nodeLeft->constrain[xMin]), RotationUtility::getDegree(nodeLeft->constrain[xMax]));
	log(LogLevel::Info, "ymin %f ymax %f", RotationUtility::getDegree(nodeLeft->constrain[yMin]), RotationUtility::getDegree(nodeLeft->constrain[yMax]));
	log(LogLevel::Info, "zmin %f zmax %f", RotationUtility::getDegree(nodeLeft->constrain[zMin]), RotationUtility::getDegree(nodeLeft->constrain[zMax]));
	
	nodeRight = bones[rightHandBoneIndex - 1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain[xMin] = nodeLeft->constrain[xMin];			nodeRight->constrain[xMax] = nodeLeft->constrain[xMax];
	nodeRight->constrain[yMin] = -nodeLeft->constrain[yMin];		nodeRight->constrain[yMax] = -nodeLeft->constrain[yMax];
	nodeRight->constrain[zMin] = -nodeLeft->constrain[zMin],		nodeRight->constrain[zMax] = -nodeLeft->constrain[zMax];
	
	log(LogLevel::Info, "Hand Right");
	log(LogLevel::Info, "xmin %f xmax %f", RotationUtility::getDegree(nodeRight->constrain[xMin]), RotationUtility::getDegree(nodeRight->constrain[xMax]));
	log(LogLevel::Info, "ymin %f ymax %f", RotationUtility::getDegree(nodeRight->constrain[yMin]), RotationUtility::getDegree(nodeRight->constrain[yMax]));
	log(LogLevel::Info, "zmin %f zmax %f", RotationUtility::getDegree(nodeRight->constrain[zMin]), RotationUtility::getDegree(nodeRight->constrain[zMax]));
	
	// Thigh
	nodeLeft = bones[leftUpLegBoneIndex - 1];
	nodeLeft->axes = Kore::vec3(1, 1, 1);
	nodeLeft->constrain[xMin] = -RotationUtility::getRadians(110) - tolerance;		nodeLeft->constrain[xMax] = RotationUtility::getRadians(30) + tolerance;
	nodeLeft->constrain[yMin] = -RotationUtility::getRadians(50) - tolerance;		nodeLeft->constrain[yMax] = RotationUtility::getRadians(40) + tolerance;
	nodeLeft->constrain[zMin] = -RotationUtility::getRadians(40) - tolerance;		nodeLeft->constrain[zMax] = RotationUtility::getRadians(0) + tolerance;
	
	log(LogLevel::Info, "Upper Leg Left");
	log(LogLevel::Info, "xmin %f xmax %f", RotationUtility::getDegree(nodeLeft->constrain[xMin]), RotationUtility::getDegree(nodeLeft->constrain[xMax]));
	log(LogLevel::Info, "ymin %f ymax %f", RotationUtility::getDegree(nodeLeft->constrain[yMin]), RotationUtility::getDegree(nodeLeft->constrain[yMax]));
	log(LogLevel::Info, "zmin %f zmax %f", RotationUtility::getDegree(nodeLeft->constrain[zMin]), RotationUtility::getDegree(nodeLeft->constrain[zMax]));
	
	nodeRight = bones[rightUpLegBoneIndex - 1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain[xMin] = nodeLeft->constrain[xMin];		nodeRight->constrain[xMax] = nodeLeft->constrain[xMax];
	nodeRight->constrain[yMin] = -nodeLeft->constrain[yMin],	nodeRight->constrain[yMax] = -nodeLeft->constrain[yMax];
	nodeRight->constrain[zMin] = -nodeLeft->constrain[zMin],	nodeRight->constrain[zMax] = -nodeLeft->constrain[zMax];
	
	log(LogLevel::Info, "Upper Leg Right");
	log(LogLevel::Info, "xmin %f xmax %f", RotationUtility::getDegree(nodeRight->constrain[xMin]), RotationUtility::getDegree(nodeRight->constrain[xMax]));
	log(LogLevel::Info, "ymin %f ymax %f", RotationUtility::getDegree(nodeRight->constrain[yMin]), RotationUtility::getDegree(nodeRight->constrain[yMax]));
	log(LogLevel::Info, "zmin %f zmax %f", RotationUtility::getDegree(nodeRight->constrain[zMin]), RotationUtility::getDegree(nodeRight->constrain[zMax]));
	
	// Calf
	nodeLeft = bones[leftLegBoneIndex - 1];
	nodeLeft->axes = Kore::vec3(1, 0, 0);
	nodeLeft->constrain[xMin] = 0;					nodeLeft->constrain[xMax] = RotationUtility::getRadians(150) + tolerance;
	
	log(LogLevel::Info, "Knee Left");
	log(LogLevel::Info, "xmin %f xmax %f", RotationUtility::getDegree(nodeLeft->constrain[xMin]), RotationUtility::getDegree(nodeLeft->constrain[xMax]));
	
	nodeRight = bones[rightLegBoneIndex - 1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain[xMin] = nodeLeft->constrain[xMin];		nodeRight->constrain[xMax] = nodeLeft->constrain[xMax];
	
	log(LogLevel::Info, "Knee Right");
	log(LogLevel::Info, "xmin %f xmax %f", RotationUtility::getDegree(nodeRight->constrain[xMin]), RotationUtility::getDegree(nodeRight->constrain[xMax]));
}
