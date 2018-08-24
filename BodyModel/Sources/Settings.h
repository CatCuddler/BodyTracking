#pragma once

extern int ikMode;

struct DataFile {
	const char* positionDataFilename;
	const float scale;
	
	DataFile(const char* position, float scale = 1.0f) : positionDataFilename(position), scale(scale) {}
};

namespace {
	enum BlockColor {
		hip, leftHand, rightHand, leftFoot, rightFoot
	};
	
	const int hipBoneIndex = 2;
	const int leftHandBoneIndex = 17;	// 17 .. 14
	const int rightHandBoneIndex = 27;	// 27 .. 24
	const int leftFootBoneIndex = 6;
	const int rightFootBoneIndex = 31;
	
	const int numOfEndEffectors = 5;
	
	DataFile* testFile = new DataFile("positionData_mix.csv", 1.08f);
	DataFile* squatsFile = new DataFile("positionData_squats.csv", 1.08f);
	DataFile* currentFile = squatsFile;
	
	const float nearNull = 0.0001f;
	const int width = 1024;
	const int height = 768;
	const bool renderRoom = false;
	const bool renderTrackerAndController = false;
	const bool renderAxisForEndEffector = true;
	const bool eval = false;
	const bool withOrientation = false;
	const float errorMaxPos = 0.01f;
	const float errorMaxRot = 0.01f;
}
