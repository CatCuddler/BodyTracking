#include "pch.h"
#include "EndEffector.h"

#include <string>

EndEffector::EndEffector(int boneIndex, IKMode ikMode) : desPosition(Kore::vec3(0, 0, 0)), desRotation(Kore::Quaternion(0, 0, 0, 1)), offsetPosition(Kore::vec3(0, 0, 0)), offsetRotation(Kore::Quaternion(0, 0, 0, 1)), boneIndex(boneIndex), deviceID(-1), ikMode(ikMode) {

	name = getNameForIndex(boneIndex);
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

const char* EndEffector::getNameForIndex(const int ID) const {
	if (ID == headBoneIndex)				return headTag;
	else if (ID == spineBoneIndex)			return spineTag;
	else if (ID == hipBoneIndex)			return hipTag;
	else if (ID == leftHandBoneIndex)		return lHandTag;
	else if (ID == rightHandBoneIndex)		return rHandTag;
	else if (ID == leftForeArmBoneIndex)	return lForeArm;
	else if (ID == rightForeArmBoneIndex)	return rForeArm;
	else if (ID == rightArmBoneIndex)		return rArm;
	else if (ID == leftFootBoneIndex)		return lFootTag;
	else if (ID == rightFootBoneIndex)		return rFootTag;
	else if (ID == leftLegBoneIndex)		return lLeg;
	else if (ID == rightLegBoneIndex)		return rLeg;
	else return nullptr;
}

int EndEffector::getIndexForName(const char* name) const {
	if(std::strcmp(name, headTag) == 0)			return headBoneIndex;
	else if(std::strcmp(name, hipTag) == 0)		return hipBoneIndex;
	else if(std::strcmp(name, lHandTag) == 0)	return leftHandBoneIndex;
	else if(std::strcmp(name, rHandTag) == 0)	return rightHandBoneIndex;
	else if(std::strcmp(name, lFootTag) == 0)	return leftFootBoneIndex;
	else if(std::strcmp(name, rFootTag) == 0)	return rightFootBoneIndex;
	else return -1;
}
