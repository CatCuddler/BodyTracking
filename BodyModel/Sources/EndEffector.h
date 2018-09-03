#pragma once

#include <Kore/Math/Vector.h>
#include <Kore/Math/Quaternion.h>

enum EndEffectorIndices {
	hip, leftHand, rightHand, leftFoot, rightFoot
};

// Hip
const int hipBoneIndex = 2;
// Left arm
const int leftShoulderBoneIndex = 11;
const int leftArmBoneIndex = 12;
const int leftForeArmBoneIndex = 13;
const int leftHandBoneIndex = 14;
// Right arm
const int rightShoulderBoneIndex = 21;
const int rightArmBoneIndex = 22;
const int rightForeArmBoneIndex = 23;
const int rightHandBoneIndex = 24;
// Left foot
const int leftUpLegBoneIndex = 4;
const int leftLegBoneIndex = 5;
const int leftFootBoneIndex = 6;
//Right foot
const int rightUpLegBoneIndex = 29;
const int rightLegBoneIndex = 30;
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
	
	Kore::vec3 getDesPosition() const;
	void setDesPosition(Kore::vec3 pos);
	
	Kore::Quaternion getDesRotation() const;
	void setDesRotation(Kore::Quaternion rot);
	
	Kore::vec3 getOffsetPosition() const;
	void setOffsetPosition(Kore::vec3 offsetPosition);
	
	Kore::Quaternion getOffsetRotation() const;
	void setOffsetRotation(Kore::Quaternion offsetRotation);
	
	int getDeviceIndex() const;
	void setDeviceIndex(int index);
	
	int getBoneIndex() const;
	const char* getName() const;
	
private:
	Kore::vec3 desPosition;
	Kore::Quaternion desRotation;
	
	Kore::vec3 offsetPosition;
	Kore::Quaternion offsetRotation;
	
	int boneIndex;		// As defined in .ogex node (e.g. nodeX ==> boneIndex = X)
	const char* name;	// Name of the end-effector (e.g. lhC)
	int deviceID;		// ID of the VR device
	int ikMode;			// 0: JT, 1: JPI, 2: DLS, 3: SVD, 4: DLS with SVD, 5: SDLS, 6: SDLS-Modified
	
	const char* getNameForIndex(const int ID) const;
	int getIndexForName(const char* name) const;

};
