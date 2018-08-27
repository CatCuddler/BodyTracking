#pragma once

#include <Kore/Math/Vector.h>
#include <Kore/Math/Quaternion.h>

enum EndEffectorIndices {
	hip, leftHand, rightHand, leftFoot, rightFoot
};

const int hipBoneIndex = 2;
const int leftHandBoneIndex = 17;	// 17 or 14
const int rightHandBoneIndex = 27;	// 27 or 24
const int leftFootBoneIndex = 6;
const int rightFootBoneIndex = 31;

const int numOfEndEffectors = 5;

class EndEffector {
public:
	EndEffector(int boneIndex, int mode = 5);
	
	Kore::vec3 getOffsetPosition() const;
	void setOffsetPosition(Kore::vec3 offsetPosition);
	
	Kore::Quaternion getOffsetRotation() const;
	void setOffsetRotation(Kore::Quaternion offsetRotation);
	
	int getTrackerIndex();
	void setTrackerIndex(int index);
	
	int getBoneIndex() const;
	
private:
	Kore::vec3 offsetPosition;
	Kore::Quaternion offsetRotation;
	int boneIndex;
	int trackerIndex;
	int ikMode; // 0: JT, 1: JPI, 2: DLS, 3: SVD, 4: DLS with SVD, 5: SDLS, 6: SDLS-Modified

};
