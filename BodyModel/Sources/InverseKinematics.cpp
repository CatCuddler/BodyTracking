#include "pch.h"
#include "InverseKinematics.h"

#include <Kore/Log.h>

InverseKinematics::InverseKinematics(std::vector<BoneNode*> boneVec) {
	bones = boneVec;
	setJointConstraints();
}

bool InverseKinematics::inverseKinematics(BoneNode* targetBone, Kore::vec4 desiredPosition, Kore::Quaternion desiredRotation) {
    std::vector<float> deltaTheta;
    float error = FLT_MAX;
    
    if (!targetBone->initialized) return false;
    
    for (int i = 0; i <= maxSteps; ++i) {
        if (targetBone->nodeIndex == 8) { // LowerBack
            deltaTheta = jacobianBack->calcDeltaTheta(targetBone, desiredPosition, desiredRotation, backIkMode);
            error = jacobianBack->getError();
        } else if (targetBone->nodeIndex == 16 || targetBone->nodeIndex == 26) { // LeftFingerBase/RightFingerBase
            deltaTheta = jacobianHand->calcDeltaTheta(targetBone, desiredPosition, desiredRotation, handIkMode);
            error = jacobianHand->getError();
        } else if (targetBone->nodeIndex == 6 || targetBone->nodeIndex == 31) { // LeftFoot/LeftHand
            deltaTheta = jacobianFoot->calcDeltaTheta(targetBone, desiredPosition, desiredRotation, footIkMode);
            error = jacobianFoot->getError();
        }
        
        // if position reached OR maxStep reached
        if (error < errorMax || i == maxSteps) {
            sumIter += i;
            sumReached += error < errorMax ? 1 : 0;
            sumError += error;
            minError = error < minError ? error : minError;
            maxError = error > maxError ? error : maxError;
            totalNum += 1;
            
            return error < errorMax;
        } else {
            applyChanges(deltaTheta, targetBone);
            applyJointConstraints(targetBone);
            for (int i = 0; i < bones.size(); ++i) updateBonePosition(bones[i]);
        }
    }
    
    return false;
}

void InverseKinematics::applyChanges(std::vector<float> deltaTheta, BoneNode* targetBone) {
    unsigned long size = deltaTheta.size();
	int i = 0;
    
    BoneNode* bone = targetBone;
    while (bone->initialized) {
        Kore::vec3 axes = bone->axes;
        
        if (axes.x() == 1.0 && i < size) bone->quaternion.rotate(Kore::Quaternion(Kore::vec3(1, 0, 0), deltaTheta[i++]));
        if (axes.y() == 1.0 && i < size) bone->quaternion.rotate(Kore::Quaternion(Kore::vec3(0, 1, 0), deltaTheta[i++]));
        if (axes.z() == 1.0 && i < size) bone->quaternion.rotate(Kore::Quaternion(Kore::vec3(0, 0, 1), deltaTheta[i++]));
        
        bone->quaternion.normalize();
        
        Kore::mat4 rotMat = bone->quaternion.matrix().Transpose();
        bone->local = bone->transform * rotMat;
        
        bone = bone->parent;
    }
}

void InverseKinematics::applyJointConstraints(BoneNode* targetBone) {
    BoneNode* bone = targetBone;
    while (bone->initialized) {
        Kore::vec3 axes = bone->axes;
        
        Kore::vec3 rot;
        Kore::RotationUtility::quatToEuler(&bone->quaternion, &rot.x(), &rot.y(), &rot.z());
        
        if (axes.x() == 1.0) clampValue(bone->constrain[0].x(), bone->constrain[0].y(), &rot.x());
        if (axes.y() == 1.0) clampValue(bone->constrain[1].x(), bone->constrain[1].y(), &rot.y());
        if (axes.z() == 1.0) clampValue(bone->constrain[2].x(), bone->constrain[2].y(), &rot.z());
        
        Kore::RotationUtility::eulerToQuat(rot.x(), rot.y(), rot.z(), &bone->quaternion);
        
        bone->quaternion.normalize();
        
        Kore::mat4 rotMat = bone->quaternion.matrix().Transpose();
        bone->local = bone->transform * rotMat;
        
        bone = bone->parent;
    }
}

bool InverseKinematics::clampValue(float minVal, float maxVal, float* value) {
	if (minVal > maxVal) {
		float temp = minVal;
		minVal = maxVal;
		maxVal = temp;
	}

	if (*value < minVal) {
		*value = minVal;
		return true;
	}
	else if (*value > maxVal) {
		*value = maxVal;
		return true;
	}
	return false;
}

void InverseKinematics::updateBonePosition(BoneNode* bone) {
	bone->combined = bone->parent->combined * bone->local;
}

void InverseKinematics::setJointConstraints() {
	BoneNode* nodeLeft;
	BoneNode* nodeRight;
    
    // hips
    nodeLeft = bones[2 - 1];
    nodeLeft->axes = Kore::vec3(1, 1, 1);
    nodeLeft->constrain.push_back(Kore::vec2(-Kore::pi, Kore::pi));
    nodeLeft->constrain.push_back(Kore::vec2(-Kore::pi, Kore::pi));
    nodeLeft->constrain.push_back(Kore::vec2(-Kore::pi, Kore::pi));

	// upperarm / Schultergelenk
	nodeLeft = bones[12 - 1];
	nodeLeft->axes = Kore::vec3(1, 1, 1);
    nodeLeft->constrain.push_back(Kore::vec2(-2.0f * Kore::pi / 9.0f, 17.0f * Kore::pi / 18.0f));   // -40° bis 170° = 210° (NN, vorher -90° bis 120° = 210° => 0°)
    nodeLeft->constrain.push_back(Kore::vec2(-Kore::pi / 2.0f, 4.0f * Kore::pi / 9.0f));            // -90° bis 80° = 170° (NN, vorher -90° bis 60° = 150° => 20°)s
    nodeLeft->constrain.push_back(Kore::vec2(-13.0f * Kore::pi / 18.0f, Kore::pi / 2.0f));          // -130° bis 90° = 220° (NN, vorher -30° bis 120° = 150° => 70°)

	nodeRight = bones[22 - 1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain.push_back(nodeLeft->constrain[0]);
	nodeRight->constrain.push_back(nodeLeft->constrain[1] * -1.0f);
	nodeRight->constrain.push_back(nodeLeft->constrain[2] * -1.0f);

	// lowerarm / Ellenbogengelenk
	nodeLeft = bones[13 - 1];
    nodeLeft->axes = Kore::vec3(1, 0, 0);
    nodeLeft->constrain.push_back(Kore::vec2(-Kore::pi / 18.0f, 3.0f * Kore::pi / 4.0f));           // -10° bis 135° = 145° (NN, vorher -90° bis 90° = 180° => -35°)

	nodeRight = bones[23 - 1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain.push_back(nodeLeft->constrain[0]);
    
    // hand
    if (handJointDOFs == 6) {
        nodeLeft = bones[14 - 1];
        nodeLeft->axes = Kore::vec3(1, 0, 1);
        nodeLeft->constrain.push_back(Kore::vec2(-2.0f * Kore::pi / 9.0f, Kore::pi / 6.0f));            // -40° bis 30° = 75° (NN)
        nodeLeft->constrain.push_back(Kore::vec2(-7.0f * Kore::pi / 18.0f, Kore::pi / 3.0f));           // -70° bis 60° = 130° (NN)
        
        nodeRight = bones[24 - 1];
        nodeRight->axes = nodeLeft->axes;
        nodeRight->constrain.push_back(nodeLeft->constrain[0]);
        nodeRight->constrain.push_back(nodeLeft->constrain[1] * -1.0f);
    }

	// thigh / Hüftgelenk
	nodeLeft = bones[4 - 1];
	nodeLeft->axes = Kore::vec3(1, 1, 1);
    nodeLeft->constrain.push_back(Kore::vec2(-13.0f * Kore::pi / 18.0f, Kore::pi / 18.0f));         // -130° bis 10° = 145° (NN, vorher -150° bis 60° = 210° => -65°)
    nodeLeft->constrain.push_back(Kore::vec2(-Kore::pi / 3.0f, 2.0f * Kore::pi / 9.0f));            // -60° bis 40° = 100° (NN, vorher -22.5° bis 22.5° = 45° => 55°)
    nodeLeft->constrain.push_back(Kore::vec2(-Kore::pi / 4.0f, 5.0f * Kore::pi / 18.0f));           // -45° bis 50° = 95° (NN, vorher -90° bis 90° = 180° => -85°)
    
	nodeRight = bones[29 - 1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain.push_back(nodeLeft->constrain[0]);
	nodeRight->constrain.push_back(nodeLeft->constrain[1] * -1.0f);
	nodeRight->constrain.push_back(nodeLeft->constrain[2] * -1.0f);

	// calf / Kniegelenk
	nodeLeft = bones[5 - 1];
	nodeLeft->axes = Kore::vec3(1, 0, 0);
    nodeLeft->constrain.push_back(Kore::vec2(-Kore::pi / 18.0f, 13.0f * Kore::pi / 18.0f));         // -10° bis 130° = 140° (NN, vorher -0° bis 150° = 150° => -10°)

	nodeRight = bones[30 - 1];
	nodeRight->axes = nodeLeft->axes;
    nodeRight->constrain.push_back(nodeLeft->constrain[0]);
}

int InverseKinematics::getTotalNum() {
    return totalNum;
}

float InverseKinematics::getAverageIter() {
    return totalNum != 0 ? (float) sumIter / (float) totalNum : -1;
}

float InverseKinematics::getAverageReached() {
    return totalNum != 0 ? (float) sumReached / (float) totalNum : -1;
}

float InverseKinematics::getAverageError() {
    return totalNum != 0 ? sumError / (float) totalNum : -1;
}

float InverseKinematics::getMinError() {
    return minError;
}

float InverseKinematics::getMaxError() {
    return maxError;
}
