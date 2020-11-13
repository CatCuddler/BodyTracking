#include "pch.h"
#include "EndEffector.h"

#include <Kore/Log.h>

#include <string>

EndEffector::EndEffector(int boneIndex, IKMode ikMode) : desPosition(Kore::vec3(0, 0, 0)), desRotation(Kore::Quaternion(0, 0, 0, 1)), offsetPosition(Kore::vec3(0, 0, 0)), offsetRotation(Kore::Quaternion(0, 0, 0, 1)), boneIndex(boneIndex), deviceID(-1), ikMode(ikMode) {
	name = getNameForIndex(boneIndex);
	
	evalErrorPos = new float[frames]();
	evalErrorRot = new float[frames]();
}

EndEffector::~EndEffector() {
	delete[] evalErrorPos;
	delete[] evalErrorRot;
}

Kore::vec3 EndEffector::getDesPosition() const {
	return desPosition;
}

void EndEffector::setDesPosition(Kore::vec3 pos) {
	desPosition = pos;
}

Kore::Quaternion EndEffector::getDesRotation() const {
	return desRotation;
}

void EndEffector::setDesRotation(Kore::Quaternion rot) {
	desRotation = rot;
}

Kore::vec3 EndEffector::getOffsetPosition() const {
	return offsetPosition;
}

void EndEffector::setOffsetPosition(Kore::vec3 pos) {
	offsetPosition = pos;
}

Kore::Quaternion EndEffector::getOffsetRotation() const {
	return offsetRotation;
}

void EndEffector::setOffsetRotation(Kore::Quaternion rot) {
	offsetRotation = rot;
}

Kore::vec3 EndEffector::getFinalPosition() const {
	return finalPosition;
}

void EndEffector::setFinalPosition(Kore::vec3 pos) {
	finalPosition = pos;
}

Kore::Quaternion EndEffector::getFinalRotation() const {
	return finalRotation;
}

void EndEffector::setFinalRotation(Kore::Quaternion rot) {
	finalRotation = rot;
}

float EndEffector::calcAvg(const float* vec) const {
	float total = 0.0f;
	for (int i = 0; i < size; i++) {
		total = total + vec[i];
	}
	return total/size;
}

float EndEffector::calcStd(const float* vec) const {
	float mean = calcAvg(vec);
	float standardDeviation = 0.0f;
	
	for (int i = 0; i < size; i++) {
        standardDeviation += Kore::pow(vec[i] - mean, 2);
	}

    return Kore::sqrt(standardDeviation / size);
}

float EndEffector::calcMin(const float* vec) const {
	float min = Kore::maxfloat();
	for (int i = 0; i < size; i++) {
		if (vec[i] < min)
			min = vec[i];
	}
	return min;
}

float EndEffector::calcMax(const float* vec) const {
	float max = 0.0f;
	for (int i = 0; i < size; i++) {
		if (vec[i] > max) max = vec[i];
	}
	return max;
}

float* EndEffector::getAvdStdMinMax(const float* vec) const {
	float *error = new float[4];
	error[0] = calcAvg(vec);
	error[1] = calcStd(vec);
	error[2] = calcMin(vec);
	error[3] = calcMax(vec);
	
	return error;
}

float EndEffector::getErrorPos() {
	return calcAvg(evalErrorPos);
}

float EndEffector::getErrorRot() {
	return calcAvg(evalErrorRot);
}

void EndEffector::getErrorPosAndRot(float& pos, float& rot) {
	pos = calcAvg(evalErrorPos);
	rot = calcAvg(evalErrorRot);
}

int EndEffector::getDeviceIndex() const {
	return deviceID;
}

void EndEffector::setDeviceIndex(int index) {
	deviceID = index;
}

int EndEffector::getBoneIndex() const {
	return boneIndex;
}

const char* EndEffector::getName() const {
	return name;
}

IKMode EndEffector::getIKMode() const {
	return ikMode;
}

void EndEffector::setIKMode(IKMode mode) {
	ikMode = mode;
}

void EndEffector::getError(BoneNode* targetBone) {
	// Get current position and rotation
	Kore::vec3 pos_current = targetBone->getPosition();
	Kore::Quaternion rot_current = targetBone->getOrientation();
	
	// Get desired position and rotation
	Kore::vec3 pos_desired = getFinalPosition();
	Kore::Quaternion rot_desired = getFinalRotation();
	
	// Calculate difference between desired position and actual position of the end effector: euclidian distance [mm]
	float posError = (pos_desired - pos_current).getLength() * 1000;
	
	// Calculate the difference between two quaternions [degree]
	rot_desired.normalize();
	Kore::Quaternion deltaRot_quat = rot_desired.rotated(rot_current.invert());
	if (deltaRot_quat.w < 0) deltaRot_quat = deltaRot_quat.scaled(-1);
	
	Kore::vec3 deltaRot = Kore::vec3(0, 0, 0);
	Kore::RotationUtility::quatToEuler(&deltaRot_quat, &deltaRot.x(), &deltaRot.y(), &deltaRot.z());
	float rotError = Kore::RotationUtility::getDegree(deltaRot.getLength());
	
	//Kore::log(Kore::LogLevel::Info, "Error for %s is posError:%f, rotError:%f", getName(), posError, rotError);
	
	// Save
	evalErrorPos[size] = posError;
	evalErrorRot[size] = rotError;
	size++;
	
	assert(size < frames);
}

void EndEffector::resetEvalVariables() {
	for (int i = 0; i < frames; i++) {
		evalErrorPos[i] = 0;
		evalErrorRot[i] = 0;
	}
	size = 0;
}

const char* EndEffector::getNameForIndex(const int ID) const {
	if (ID == headBoneIndex)				return headTag;
	else if (ID == hipBoneIndex)			return hipTag;
	else if (ID == leftHandBoneIndex)		return lHandTag;
	else if (ID == rightHandBoneIndex)		return rHandTag;
	else if (ID == leftForeArmBoneIndex)	return lForeArm;
	else if (ID == rightForeArmBoneIndex)	return rForeArm;
	else if (ID == leftFootBoneIndex)		return lFootTag;
	else if (ID == rightFootBoneIndex)		return rFootTag;
	else if (ID == leftLegBoneIndex)		return lKneeTag;
	else if (ID == rightLegBoneIndex)		return rKneeTag;
	else return nullptr;
}

int EndEffector::getIndexForName(const char* name) const {
	if(std::strcmp(name, headTag) == 0)			return headBoneIndex;
	else if(std::strcmp(name, hipTag) == 0)		return hipBoneIndex;
	else if(std::strcmp(name, lHandTag) == 0)	return leftHandBoneIndex;
	else if(std::strcmp(name, rHandTag) == 0)	return rightHandBoneIndex;
	else if (std::strcmp(name, lForeArm) == 0)	return leftForeArmBoneIndex;
	else if (std::strcmp(name, rForeArm) == 0)	return rightForeArmBoneIndex;
	else if (std::strcmp(name, lFootTag) == 0)	return leftFootBoneIndex;
	else if (std::strcmp(name, rFootTag) == 0)	return rightFootBoneIndex;
	else if (std::strcmp(name, lKneeTag) == 0)	return leftLegBoneIndex;
	else if (std::strcmp(name, rKneeTag) == 0)	return rightLegBoneIndex;
	else return -1;
}
