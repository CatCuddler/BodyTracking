#include "pch.h"
#include "InverseKinematics.h"
#include "EndEffector.h"

#include <float.h>
#include <Kore/Log.h>

InverseKinematics::InverseKinematics(std::vector<BoneNode*> boneVec) {
	bones = boneVec;
	setJointConstraints();
	
	totalNum = 0;
	evalReached = 0;
	evalStucked = 0;
	
	// iterations
	evalIterations[0] = 0;
	evalIterations[1] = FLT_MAX;
	evalIterations[2] = FLT_MIN;
	
	// pos-error
	evalErrorPos[0] = 0;
	evalErrorPos[1] = FLT_MAX;
	evalErrorPos[2] = FLT_MIN;
	
	// rot-error
	evalErrorRot[0] = 0;
	evalErrorRot[1] = FLT_MAX;
	evalErrorRot[2] = FLT_MIN;
	
	// time
	evalTime[0] = 0;
	evalTime[1] = FLT_MAX;
	evalTime[2] = FLT_MIN;
	
	// time per iteration
	evalTimeIteration[0] = 0;
	evalTimeIteration[1] = FLT_MAX;
	evalTimeIteration[2] = FLT_MIN;
}

void InverseKinematics::inverseKinematics(BoneNode* targetBone, Kore::vec3 desPosition, Kore::Quaternion desRotation) {
	std::vector<float> deltaTheta, prevDeltaTheta;
	float errorPos = -1.0f;
	float errorRot = -1.0f;
	bool stucked = false;
#ifdef KORE_MACOS
	struct timespec start, end;
	struct timespec start2, end2;
	
	if (eval) clock_gettime(CLOCK_MONOTONIC_RAW, &start);
#endif
	
	int i = 0;
	// while position not reached and maxStep not reached and not stucked
	while ((errorPos < 0 || errorPos > errorMaxPos || errorRot < 0 || errorRot > errorMaxRot) && i < (int) maxSteps[ikMode] && !stucked) {
#ifdef KORE_MACOS
		if (eval) clock_gettime(CLOCK_MONOTONIC_RAW, &start2);
#endif
		
		prevDeltaTheta = deltaTheta;
		
		// todo: better!
		if (targetBone->nodeIndex == leftHandBoneIndex || targetBone->nodeIndex == rightHandBoneIndex) {
			// deltaTheta = jacobianHand->calcDeltaTheta(targetBone, desPosition, desRotation, tracker[1]->ikMode);
			deltaTheta = jacobianHand->calcDeltaTheta(targetBone, desPosition, desRotation, ikMode); // todo: remove after eval
			errorPos = jacobianHand->getPositionError();
			errorRot = jacobianHand->getRotationError();
		} else if (targetBone->nodeIndex == leftFootBoneIndex|| targetBone->nodeIndex == rightFootBoneIndex) {
			// deltaTheta = jacobianFoot->calcDeltaTheta(targetBone, desPosition, desRotation, tracker[3]->ikMode);
			deltaTheta = jacobianFoot->calcDeltaTheta(targetBone, desPosition, desRotation, ikMode); // todo: remove after eval
			errorPos = jacobianFoot->getPositionError();
			errorRot = jacobianFoot->getRotationError();
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
		
#ifdef KORE_MACOS
		if (eval && i == 0) {
			// time per iteration
			clock_gettime(CLOCK_MONOTONIC_RAW, &end2);
			float time = (end2.tv_sec - start2.tv_sec) * 1000000 + (end2.tv_nsec - start2.tv_nsec) / 1000;
			evalTimeIteration[0] += time;
			evalTimeIteration[1] = time < evalTimeIteration[1] ? time : evalTimeIteration[1];
			evalTimeIteration[2] = time > evalTimeIteration[2] ? time : evalTimeIteration[2];
		}
#endif // KORE_MACOS
		
		i++;
	}
	
	if (eval) {
		totalNum += 1;
		evalReached += (errorPos < errorMaxPos && errorRot < errorMaxRot) ? 1 : 0;
		evalStucked += stucked ? 1 : 0;
		
		// iterations
		evalIterations[0] += i;
		evalIterations[1] = i < evalIterations[1] ? (float) i : evalIterations[1];
		evalIterations[2] = i > evalIterations[2] ? (float) i : evalIterations[2];
		
		// pos-error
		evalErrorPos[0] += errorPos > 0 ? errorPos : 0;
		evalErrorPos[1] = errorPos < evalErrorPos[1] ? errorPos : evalErrorPos[1];
		evalErrorPos[2] = errorPos > evalErrorPos[2] ? errorPos : evalErrorPos[2];
		
		// rot-error
		evalErrorRot[0] += errorRot > 0 ? errorRot : 0;
		evalErrorRot[1] = errorRot < evalErrorRot[1] ? errorRot : evalErrorRot[1];
		evalErrorRot[2] = errorRot > evalErrorRot[2] ? errorRot : evalErrorRot[2];
		
#ifdef KORE_MACOS
		clock_gettime(CLOCK_MONOTONIC_RAW, &end);
		
		// time
		float time = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
		evalTime[0] += time;
		evalTime[1] = time < evalTime[1] ? time : evalTime[1];
		evalTime[2] = time > evalTime[2] ? time : evalTime[2];
#endif
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
		
		int i = 0;
		float x = rot.x(), y = rot.y(), z = rot.z();
		if (axes.x() == 1.0) {
			x = clampValue(bone->constrain[i].x(), bone->constrain[i].y(), rot.x());
			i++;
		}
		if (axes.y() == 1.0) {
			y = clampValue(bone->constrain[i].x(), bone->constrain[i].y(), rot.y());
			i++;
		}
		if (axes.z() == 1.0) {
			z = clampValue(bone->constrain[i].x(), bone->constrain[i].y(), rot.z());
			i++;
		}
		Kore::RotationUtility::eulerToQuat(x, y, z, &bone->rotation);
		
		// bone->rotation = Kore::Quaternion((double) x, (double) y, (double) z, 1);
		bone->rotation.normalize();
		bone->local = bone->transform * bone->rotation.matrix().Transpose();
		bone = bone->parent;
	}
}

float InverseKinematics::clampValue(float minVal, float maxVal, float value) {
	if (minVal > maxVal) {
		float temp = minVal;
		minVal = maxVal;
		maxVal = temp;
	}
	
	if (value < minVal)
		return minVal;
	else if (value > maxVal)
		return maxVal;
	
	return value;
}

float InverseKinematics::getRadian(float degree) {
	return degree * (Kore::pi / 180.0f);
}

void InverseKinematics::setJointConstraints() {
	BoneNode* nodeLeft;
	BoneNode* nodeRight;
	
	float tolerance = getRadian(15);
	
	// Upperarm
	nodeLeft = bones[leftArmBoneIndex - 1];
	nodeLeft->axes = Kore::vec3(1, 1, 1);
	nodeLeft->constrain.push_back(Kore::vec2(-getRadian(50) - tolerance, getRadian(180) + tolerance));
	nodeLeft->constrain.push_back(Kore::vec2(-getRadian(90) - tolerance, getRadian(90) + tolerance));
	nodeLeft->constrain.push_back(Kore::vec2(-getRadian(130) - tolerance, getRadian(90) + tolerance));
	
	nodeRight = bones[rightArmBoneIndex - 1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain.push_back(nodeLeft->constrain[0]);
	nodeRight->constrain.push_back(nodeLeft->constrain[1] * -1.0f);
	nodeRight->constrain.push_back(nodeLeft->constrain[2] * -1.0f);
	
	// Forearm
	nodeLeft = bones[leftForeArmBoneIndex - 1];
	nodeLeft->axes = Kore::vec3(1, 0, 0);
	nodeLeft->constrain.push_back(Kore::vec2(0, getRadian(140) + tolerance));
	
	nodeRight = bones[rightForeArmBoneIndex - 1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain.push_back(nodeLeft->constrain[0]);
	
	// Hand TODO
	nodeLeft = bones[14 - 1];
	nodeLeft->axes = Kore::vec3(1, 0, 1);
	nodeLeft->constrain.push_back(Kore::vec2(-getRadian(40) - tolerance, getRadian(30) + tolerance));
	nodeLeft->constrain.push_back(Kore::vec2(-getRadian(70) - tolerance, getRadian(60) + tolerance));
	
	nodeRight = bones[24 - 1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain.push_back(nodeLeft->constrain[0]);
	nodeRight->constrain.push_back(nodeLeft->constrain[1] * -1.0f);
	
	// Thigh
	nodeLeft = bones[leftUpLegBoneIndex - 1];
	nodeLeft->axes = Kore::vec3(1, 1, 1);
	nodeLeft->constrain.push_back(Kore::vec2(-getRadian(130) - tolerance, getRadian(30) + tolerance));
	nodeLeft->constrain.push_back(Kore::vec2(-getRadian(60) - tolerance, getRadian(40) + tolerance));
	nodeLeft->constrain.push_back(Kore::vec2(-getRadian(50) - tolerance, getRadian(50) + tolerance));
	
	nodeRight = bones[rightUpLegBoneIndex - 1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain.push_back(nodeLeft->constrain[0]);
	nodeRight->constrain.push_back(nodeLeft->constrain[1] * -1.0f);
	nodeRight->constrain.push_back(nodeLeft->constrain[2] * -1.0f);
	
	// Calf
	nodeLeft = bones[leftLegBoneIndex - 1];
	nodeLeft->axes = Kore::vec3(1, 0, 0);
	nodeLeft->constrain.push_back(Kore::vec2(0, getRadian(140) + tolerance));
	
	nodeRight = bones[rightLegBoneIndex - 1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain.push_back(nodeLeft->constrain[0]);
}

float InverseKinematics::getReached() {
	return totalNum != 0 ? (float) evalReached / (float) totalNum : -1;
}

float InverseKinematics::getStucked() {
	return totalNum != 0 ? (float) evalStucked / (float) totalNum : -1;
}

float* InverseKinematics::getIterations() {
	evalIterations[0] = totalNum != 0 ? (float) evalIterations[0] / (float) totalNum : -1;
	
	return evalIterations;
}

float* InverseKinematics::getErrorPos() {
	evalErrorPos[0] = totalNum != 0 ? evalErrorPos[0] / (float) totalNum : -1;
	
	return evalErrorPos;
}

float* InverseKinematics::getErrorRot() {
	evalErrorRot[0] = totalNum != 0 ? evalErrorRot[0] / (float) totalNum : -1;
	
	return evalErrorRot;
}

float* InverseKinematics::getTime() {
	evalTime[0] = totalNum != 0 ? evalTime[0] / (float) totalNum : -1;
	
	return evalTime;
}

float* InverseKinematics::getTimeIteration() {
	evalTimeIteration[0] = totalNum != 0 ? evalTimeIteration[0] / (float) totalNum : -1;
	
	return evalTimeIteration;
}
