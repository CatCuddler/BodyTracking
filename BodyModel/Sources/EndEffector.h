#pragma once

#include <Kore/Math/Vector.h>
#include <Kore/Math/Quaternion.h>

enum EndEffectorIndices {
	head, hip, leftHand, rightHand, leftFoot, rightFoot
};

enum IKMode {
	JT, JPI, DLS, SVD, SVD_DLS, SDLS, SDLS_Mod
};

// Head
const int headBoneIndex = 20;
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
// Right foot
const int rightUpLegBoneIndex = 29;
const int rightLegBoneIndex = 30;
const int rightFootBoneIndex = 31;

// Tags used in .csv
const char* const headTag = "head";
const char* const hipTag = "hip";
const char* const lHandTag = "lHand";
const char* const rHandTag = "rHand";
const char* const lFootTag = "lFoot";
const char* const rFootTag = "rFoot";

const int numOfEndEffectors = 6;

class EndEffector {
public:
	EndEffector(int boneIndex, IKMode ikMode = JT);
	
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
	
	IKMode getIKMode() const;
	void setIKMode(IKMode mode);
	
private:
	Kore::vec3 desPosition;
	Kore::Quaternion desRotation;
	
	Kore::vec3 offsetPosition;
	Kore::Quaternion offsetRotation;
	
	int boneIndex;		// As defined in .ogex node (e.g. nodeX ==> boneIndex = X)
	const char* name;	// Name of the end-effector (e.g. lhC)
	int deviceID;		// ID of the VR device
	IKMode ikMode;
	
	const char* getNameForIndex(const int ID) const;
	int getIndexForName(const char* name) const;

};
