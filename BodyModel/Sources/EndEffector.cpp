#include "pch.h"

#include "EndEffector.h"

EndEffector::EndEffector(int boneIndex, int mode) : offsetPosition(Kore::vec3(0, 0, 0)), offsetRotation(Kore::Quaternion(0, 0, 0, 1)), boneIndex(boneIndex), trackerIndex(-1), ikMode(mode) {

}

Kore::vec3 EndEffector::getOffsetPosition() const {
	return offsetPosition;
}

Kore::Quaternion EndEffector::getOffsetRotation() const {
	return offsetRotation;
}

void EndEffector::setOffsetPosition(Kore::vec3 pos) {
	offsetPosition = pos;
}

void EndEffector::setOffsetRotation(Kore::Quaternion rot) {
	offsetRotation = rot;
}

int EndEffector::getTrackerIndex() {
	return trackerIndex;
}

void EndEffector::setTrackerIndex(int index) {
	trackerIndex = index;
}

int EndEffector::getBoneIndex() const {
	return boneIndex;
}
