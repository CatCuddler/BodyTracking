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

const char* const hipT = "hip";	// Hip tracker
const char* const lhC = "lhc";	// Left hand controller
const char* const rhC = "rhc";	// Right hand controller
const char* const lfT = "lft";	// Left foot tracker*/
const char* const rfT = "rft";	// Right foot tracker

const int numOfEndEffectors = 5;

class EndEffector {
public:
	EndEffector(int boneIndex, int mode = 5);
	
	Kore::vec3 getOffsetPosition() const;
	void setOffsetPosition(Kore::vec3 offsetPosition);
	
	Kore::Quaternion getOffsetRotation() const;
	void setOffsetRotation(Kore::Quaternion offsetRotation);
	
	int getTrackerIndex() const;
	void setTrackerIndex(int index);
	
	int getBoneIndex() const;
	const char* getName() const;
	
private:
	Kore::vec3 offsetPosition;
	Kore::Quaternion offsetRotation;
	int boneIndex;		// As defined in .ogex node (e.g. nodeX ==> boneIndex = X)
	const char* name;	// Name of the end-effector (e.g. lhC)
	int trackerIndex;	// ID of the VR device
	int ikMode;			// 0: JT, 1: JPI, 2: DLS, 3: SVD, 4: DLS with SVD, 5: SDLS, 6: SDLS-Modified
	
	const char* getNameForIndex(const int ID) const;
	int getIndexForName(const char* name) const;

};
