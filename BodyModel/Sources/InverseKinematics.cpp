#include "pch.h"
#include "InverseKinematics.h"
#include "RotationUtility.h"

using namespace Kore;

extern float errorMaxPos[];
extern float errorMaxRot[];
extern float maxIterations[];

InverseKinematics::InverseKinematics(std::vector<BoneNode*> boneVec) {
	bones = boneVec;
	setJointConstraints();
	
	setEvalVariables();
}

void InverseKinematics::inverseKinematics(BoneNode* targetBone, IKMode ikMode, Kore::vec3 desPosition, Kore::Quaternion desRotation) {
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
	while ((errorPos < 0 || errorPos > errorMaxPos[ikMode] || errorRot < 0 || errorRot > errorMaxRot[ikMode]) && i < (int) maxIterations[ikMode] && !stucked) {
#ifdef KORE_MACOS
		if (eval) clock_gettime(CLOCK_MONOTONIC_RAW, &start2);
#endif
		
		prevDeltaTheta = deltaTheta;
		
		// todo: better!
		if ((!simpleIK && (targetBone->nodeIndex == leftForeArmBoneIndex || targetBone->nodeIndex == rightForeArmBoneIndex)) ||
			 (simpleIK && (targetBone->nodeIndex == leftHandBoneIndex || targetBone->nodeIndex == rightHandBoneIndex))) {
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
		
#ifdef KORE_MACOS
		if (eval && i == 0) {
			// time per iteration
			clock_gettime(CLOCK_MONOTONIC_RAW, &end2);
			float time = (end2.tv_sec - start2.tv_sec) * 1000000 + (end2.tv_nsec - start2.tv_nsec) / 1000;
			evalTimeIteration[totalNum] = time;
		}
#endif // KORE_MACOS
		
		i++;
	}
	
	if (eval) {
		evalReached += (errorPos < errorMaxPos[ikMode] && errorRot < errorMaxRot[ikMode]) ? 1 : 0;
		evalStucked += stucked ? 1 : 0;
		
		// iterations
		evalIterations[totalNum] = (float) i;
		
		// pos-error
		evalErrorPos[totalNum] = errorPos > 0 ? errorPos : 0;
		
		// rot-error
		evalErrorRot[totalNum] = errorRot > 0 ? errorRot : 0;
		
#ifdef KORE_MACOS
		clock_gettime(CLOCK_MONOTONIC_RAW, &end);
		
		// time
		float time = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
		evalTime[totalNum] = time;
#endif
		
		totalNum++;
		assert(totalNum < frames);
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

void InverseKinematics::setJointConstraints() {
	BoneNode* nodeLeft;
	BoneNode* nodeRight;
	
	float tolerance = RotationUtility::getRadians(15);
	
	// Neck
	nodeLeft = bones[neckBoneIndex - 1];
	nodeLeft->axes = Kore::vec3(1, 1, 1);
	nodeLeft->constrain[xMin] = -RotationUtility::getRadians(45) - tolerance;		nodeLeft->constrain[xMax] = RotationUtility::getRadians(45) + tolerance;
	nodeLeft->constrain[yMin] = -RotationUtility::getRadians(45) - tolerance;		nodeLeft->constrain[yMax] = RotationUtility::getRadians(45) + tolerance;
	nodeLeft->constrain[zMin] = -RotationUtility::getRadians(45) - tolerance;		nodeLeft->constrain[zMax] = RotationUtility::getRadians(45) + tolerance;
	
	// Upper body
/*	nodeLeft = bones[upperBack - 1];
	nodeLeft->axes = Kore::vec3(1, 1, 1);
	nodeLeft->constrain[xMin] = -RotationUtility::getRadians(30) - tolerance;		nodeLeft->constrain[xMax] = RotationUtility::getRadians(30) + tolerance;
	nodeLeft->constrain[yMin] = -RotationUtility::getRadians(20) - tolerance;		nodeLeft->constrain[yMax] = RotationUtility::getRadians(20) + tolerance;
	nodeLeft->constrain[zMin] = -RotationUtility::getRadians(20) - tolerance;		nodeLeft->constrain[zMax] = RotationUtility::getRadians(20) + tolerance;*/
	
	// Upperarm
	nodeLeft = bones[leftArmBoneIndex - 1];
	nodeLeft->axes = Kore::vec3(1, 1, 1);
	nodeLeft->constrain[xMin] = -RotationUtility::getRadians(50) - tolerance;		nodeLeft->constrain[xMax] = RotationUtility::getRadians(180) + tolerance;
	nodeLeft->constrain[yMin] = -RotationUtility::getRadians(90) - tolerance;		nodeLeft->constrain[yMax] = RotationUtility::getRadians(90) + tolerance;
	nodeLeft->constrain[zMin] = -RotationUtility::getRadians(90) - tolerance;		nodeLeft->constrain[zMax] = RotationUtility::getRadians(90) + tolerance;
	
	nodeRight = bones[rightArmBoneIndex - 1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain[xMin] = nodeLeft->constrain[xMin];		nodeRight->constrain[xMax] = nodeLeft->constrain[xMax];
	nodeRight->constrain[yMin] = -nodeLeft->constrain[yMin],	nodeRight->constrain[yMax] = -nodeLeft->constrain[yMax];
	nodeRight->constrain[zMin] = -nodeLeft->constrain[zMin],	nodeRight->constrain[zMax] = -nodeLeft->constrain[zMax];
	
	// Forearm
	nodeLeft = bones[leftForeArmBoneIndex - 1];
	nodeLeft->axes = Kore::vec3(1, 0, 0);
	nodeLeft->constrain[xMin] = -RotationUtility::getRadians(10) - tolerance;		nodeLeft->constrain[xMax] = RotationUtility::getRadians(140) + tolerance;
	
	nodeRight = bones[rightForeArmBoneIndex - 1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain[xMin] = nodeLeft->constrain[xMin];		nodeRight->constrain[xMax] = nodeLeft->constrain[xMax];
	
	// Hand
	nodeLeft = bones[leftHandBoneIndex - 1];
	//nodeLeft->axes = Kore::vec3(1, 1, 1);
	nodeLeft->axes = Kore::vec3(1, 0, 1);
	nodeLeft->constrain[xMin] = -RotationUtility::getRadians(20) - tolerance;		nodeLeft->constrain[xMax] = RotationUtility::getRadians(30) + tolerance;
	//nodeLeft->constrain[yMin] = -RotationUtility::getRadians(80) - tolerance;		nodeLeft->constrain[yMax] = RotationUtility::getRadians(80) + tolerance;
	nodeLeft->constrain[zMin] = -RotationUtility::getRadians(60) - tolerance;		nodeLeft->constrain[zMax] = RotationUtility::getRadians(60) + tolerance;
	
	nodeRight = bones[rightHandBoneIndex - 1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain[xMin] = nodeLeft->constrain[xMin];		nodeRight->constrain[xMax] = nodeLeft->constrain[xMax];
	//nodeRight->constrain[yMin] = nodeLeft->constrain[yMin];		nodeRight->constrain[yMax] = nodeLeft->constrain[yMax];
	nodeRight->constrain[zMin] = -nodeLeft->constrain[zMin],	nodeRight->constrain[zMax] = -nodeLeft->constrain[zMax];
	
	// Thigh
	nodeLeft = bones[leftUpLegBoneIndex - 1];
	nodeLeft->axes = Kore::vec3(1, 1, 1);
	nodeLeft->constrain[xMin] = -RotationUtility::getRadians(110) - tolerance;		nodeLeft->constrain[xMax] = RotationUtility::getRadians(30) + tolerance;
	nodeLeft->constrain[yMin] = -RotationUtility::getRadians(60) - tolerance;		nodeLeft->constrain[yMax] = RotationUtility::getRadians(40) + tolerance;
	nodeLeft->constrain[zMin] = -RotationUtility::getRadians(50) - tolerance;		nodeLeft->constrain[zMax] = RotationUtility::getRadians(20) + tolerance;
	
	nodeRight = bones[rightUpLegBoneIndex - 1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain[xMin] = nodeLeft->constrain[xMin];		nodeRight->constrain[xMax] = nodeLeft->constrain[xMax];
	nodeRight->constrain[yMin] = -nodeLeft->constrain[yMin],	nodeRight->constrain[yMax] = -nodeLeft->constrain[yMax];
	nodeRight->constrain[zMin] = -nodeLeft->constrain[zMin],	nodeRight->constrain[zMax] = -nodeLeft->constrain[zMax];
	
	// Calf
	nodeLeft = bones[leftLegBoneIndex - 1];
	nodeLeft->axes = Kore::vec3(1, 0, 0);
	nodeLeft->constrain[xMin] = 0;					nodeLeft->constrain[xMax] = RotationUtility::getRadians(150) + tolerance;
	
	nodeRight = bones[rightLegBoneIndex - 1];
	nodeRight->axes = nodeLeft->axes;
	nodeRight->constrain[xMin] = nodeLeft->constrain[xMin];		nodeRight->constrain[xMax] = nodeLeft->constrain[xMax];
}

void InverseKinematics::setEvalVariables() {
	totalNum = 0;
	evalReached = 0;
	evalStucked = 0;
	
	evalIterations = new float[frames]();
	evalErrorPos = new float[frames]();
	evalErrorRot = new float[frames]();
	evalTime = new float[frames]();
	evalTimeIteration = new float[frames]();
}

float InverseKinematics::calcAvg(const float* vec) const {
	float total = 0.0f;
	for (int i = 0; i < totalNum; i++) {
		total = total + vec[i];
	}
	return total/totalNum;
}

float InverseKinematics::calcStd(const float* vec) const {
	float mean = calcAvg(vec);
	float standardDeviation = 0.0f;
	
	for(int i = 0; i < totalNum; i++) {
        standardDeviation += Kore::pow(vec[i] - mean, 2);
	}

    return Kore::sqrt(standardDeviation / totalNum);
}

float InverseKinematics::calcMin(const float* vec) const {
	float min = maxfloat();
	for (int i = 0; i < totalNum; i++) {
		if (vec[i] < min)
			min = vec[i];
	}
	return min;
}

float InverseKinematics::calcMax(const float* vec) const {
	float max = 0.0f;
	for (int i = 0; i < totalNum; i++) {
		if (vec[i] > max) max = vec[i];
	}
	return max;
}

float InverseKinematics::getReached() {
	return totalNum != 0 ? (float) evalReached / (float) totalNum : -1;
}

float InverseKinematics::getStucked() {
	return totalNum != 0 ? (float) evalStucked / (float) totalNum : -1;
}

float* InverseKinematics::getIterations() {
	float *it = new float[4];
	it[0] = calcAvg(evalIterations);
	it[1] = calcStd(evalIterations);
	it[2] = calcMin(evalIterations);
	it[3] = calcMax(evalIterations);
	
	return it;
}

float* InverseKinematics::getErrorPos() {
	float *error = new float[4];
	error[0] = calcAvg(evalErrorPos);
	error[1] = calcStd(evalErrorPos);
	error[2] = calcMin(evalErrorPos);
	error[3] = calcMax(evalErrorPos);
	
	return error;
}

float* InverseKinematics::getErrorRot() {
	float *error = new float[4];
	error[0] = calcAvg(evalErrorRot);
	error[1] = calcStd(evalErrorRot);
	error[2] = calcMin(evalErrorRot);
	error[3] = calcMax(evalErrorRot);
	
	return error;
}

float* InverseKinematics::getTime() {
	float *time = new float[4];
	time[0] = calcAvg(evalTime);
	time[1] = calcStd(evalTime);
	time[2] = calcMin(evalTime);
	time[3] = calcMax(evalTime);
	
	return time;
}

float* InverseKinematics::getTimeIteration() {
	float *timeIt = new float[4];
	timeIt[0] = calcAvg(evalTimeIteration);
	timeIt[1] = calcStd(evalTimeIteration);
	timeIt[2] = calcMin(evalTimeIteration);
	timeIt[3] = calcMax(evalTimeIteration);
	
	return timeIt;
}
