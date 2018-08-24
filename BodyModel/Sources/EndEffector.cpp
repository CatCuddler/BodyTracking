#include "pch.h"

#include "EndEffector.h"

EndEffector::EndEffector(int boneIndex, int mode) : offsetPosition(Kore::vec3(0, 0, 0)), offsetRotation(Kore::Quaternion(0, 0, 0, 1)), boneIndex(boneIndex), trackerIndex(-1), ikMode(mode) {

}

void EndEffector::setTrackerIndex(int index) {
	trackerIndex = index;
}

int EndEffector::getBoneIndex() const {
	return boneIndex;
}
