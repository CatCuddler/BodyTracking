#include "pch.h"
#include "InverseKinematics.h"

#include "RotationUtility.h"
#include "MeshObject.h"
#include "Jacobian.h"

#include <Kore/Log.h>

InverseKinematics::InverseKinematics(std::vector<BoneNode*> boneVec, int maxSteps) : maxSteps(maxSteps), maxError(0.01f), rootIndex(2), sumIter(0), totalNum(0) {
	bones = boneVec;
	setJointConstraints();
}

bool InverseKinematics::inverseKinematics(BoneNode* targetBone, Kore::vec4 desiredPosition, Kore::Quaternion desiredRotation) {
    if (!targetBone->initialized) return false;
    
    BoneNode* bone = targetBone;
    int counter = 0;
    while (bone->initialized) {
        Kore::vec3 axes = bone->axes;
        
        if (axes.x() == 1.0) counter += 1;
        if (axes.y() == 1.0) counter += 1;
        if (axes.z() == 1.0) counter += 1;
        
        bone = bone->parent;
    }
    if (counter != 7 && counter != 9)
        log(Kore::Info, "Die Anzahl der Gelenke-Freiheitsgrade von %s ist %i", targetBone->boneName, counter);
    
    Jacobian* jacobian = new Jacobian(targetBone, desiredPosition, desiredRotation, counter);
    
    for (int i = 0; i < maxSteps; ++i) {
        // if position had reached
        if (jacobian->getError() < maxError) {
            sumIter += i;
            ++totalNum;
            
            return true;
        }
        
        applyChanges(jacobian->calcDeltaTheta(), targetBone);
        applyJointConstraints(targetBone);
        for (int i = 0; i < bones.size(); ++i) updateBonePosition(bones[i]);
    }
    
    sumIter += maxSteps;
    ++totalNum;
    
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

	// shoulder
	nodeLeft = bones[11 - 1];
	nodeLeft->axes = Kore::vec4(0, 0, 0, 0);
	nodeLeft->constrain.push_back(Kore::vec2(-0.05, 0.05));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));

	nodeRight = bones[21 - 1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain = nodeLeft->constrain;

	// upperarm
	nodeLeft = bones[12 - 1];
	nodeLeft->axes = Kore::vec3(1, 1, 1);
	nodeLeft->constrain.push_back(Kore::vec2(-Kore::pi / 2.0f, 2.0f * Kore::pi / 3.0f));
	nodeLeft->constrain.push_back(Kore::vec2(-Kore::pi / 2.0f, Kore::pi / 3.0f));
	nodeLeft->constrain.push_back(Kore::vec2(-Kore::pi / 6.0f, 2.0f * Kore::pi / 3.0f));

	nodeRight = bones[22 - 1];
	nodeRight->axes = nodeLeft->axes;
	//nodeRight->constrain = nodeLeft->constrain;
	nodeRight->constrain.push_back(nodeLeft->constrain[0]);
	nodeRight->constrain.push_back(nodeLeft->constrain[1] * -1.0f);
	nodeRight->constrain.push_back(nodeLeft->constrain[2] * -1.0f);

	// lowerarm
	nodeLeft = bones[13 - 1];
	nodeLeft->axes = Kore::vec3(1, 1, 1);
	nodeLeft->constrain.push_back(Kore::vec2(-Kore::pi, Kore::pi));//-Kore::pi / 6.0f, 2.0f * Kore::pi / 3.0f));
	nodeLeft->constrain.push_back(Kore::vec2(-Kore::pi, Kore::pi));//-Kore::pi / 3.0f, Kore::pi / 6.0f));
	nodeLeft->constrain.push_back(Kore::vec2(-Kore::pi, Kore::pi));//-Kore::pi / 8.0f, Kore::pi / 8.0f));

	nodeRight = bones[23 - 1];
	nodeRight->axes = nodeLeft->axes;
	//nodeRight->constrain = nodeLeft->constrain;
	nodeRight->constrain.push_back(nodeLeft->constrain[0]);
	nodeRight->constrain.push_back(nodeLeft->constrain[1] * -1.0f);
	nodeRight->constrain.push_back(nodeLeft->constrain[2] * -1.0f);


	// thigh
	nodeLeft = bones[4 - 1];
	nodeLeft->axes = Kore::vec3(1, 1, 1);
	nodeLeft->constrain.push_back(Kore::vec2(-5.0 * Kore::pi / 6.0f, Kore::pi / 3.0f));
	nodeLeft->constrain.push_back(Kore::vec2(-Kore::pi / 8.0f, Kore::pi / 8.0f));
	nodeLeft->constrain.push_back(Kore::vec2(-Kore::pi / 2.0f, Kore::pi / 2.0f));

	nodeRight = bones[29 - 1];
	nodeRight->axes = nodeLeft->axes;
	//nodeRight->constrain = nodeLeft->constrain;
	nodeRight->constrain.push_back(nodeLeft->constrain[0]);
	nodeRight->constrain.push_back(nodeLeft->constrain[1] * -1.0f);
	nodeRight->constrain.push_back(nodeLeft->constrain[2] * -1.0f);

	// calf
	nodeLeft = bones[5 - 1];
	nodeLeft->axes = Kore::vec3(1, 0, 0);
	nodeLeft->constrain.push_back(Kore::vec2(0, 5.0 * Kore::pi / 6.0f));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));

	nodeRight = bones[30 - 1];
	nodeRight->axes = nodeLeft->axes;
	//nodeRight->constrain = nodeLeft->constrain;
	nodeRight->constrain.push_back(nodeLeft->constrain[0]);
	nodeRight->constrain.push_back(nodeLeft->constrain[1] * -1.0f);
	nodeRight->constrain.push_back(nodeLeft->constrain[2] * -1.0f);

	// foot
	nodeLeft = bones[6 - 1];
	nodeLeft->axes = Kore::vec3(0, 0, 0);
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));
	nodeLeft->constrain.push_back(Kore::vec2(0, 0));

	nodeRight = bones[31 - 1];
	nodeRight->axes = nodeLeft->axes;
	//nodeRight->constrain = nodeLeft->constrain;
	nodeRight->constrain.push_back(nodeLeft->constrain[0]);
	nodeRight->constrain.push_back(nodeLeft->constrain[1] * -1.0f);
	nodeRight->constrain.push_back(nodeLeft->constrain[2] * -1.0f);
}

float InverseKinematics::getAverageIter() {
	float average = -1;
	if (totalNum != 0) average = sumIter / (float)totalNum;
	return average;
}
