#include "pch.h"

#include "EndEffector.h"

#include <string>

EndEffector::EndEffector(int boneIndex, int mode) : offsetPosition(Kore::vec3(0, 0, 0)), offsetRotation(Kore::Quaternion(0, 0, 0, 1)), boneIndex(boneIndex), deviceID(-1), ikMode(mode) {

	name = getNameForIndex(boneIndex);
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

const char* EndEffector::getNameForIndex(const int ID) const {
	if(ID == hipBoneIndex)				return hipT;
	else if(ID == leftHandBoneIndex)	return lhC;
	else if(ID == rightHandBoneIndex)	return rhC;
	else if(ID == leftFootBoneIndex)	return lfT;
	else if(ID == rightFootBoneIndex)	return lfT;
	else return nullptr;
}

int EndEffector::getIndexForName(const char* name) const {
	if(std::strcmp(name, hipT) == 0)		return hipBoneIndex;
	else if(std::strcmp(name, lhC) == 0)	return leftHandBoneIndex;
	else if(std::strcmp(name, rhC) == 0)	return rightHandBoneIndex;
	else if(std::strcmp(name, lfT) == 0)	return leftFootBoneIndex;
	else if(std::strcmp(name, rfT) == 0)	return rightFootBoneIndex;
	else return -1;
}
