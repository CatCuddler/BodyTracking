#pragma once

extern int ikMode;

struct EndEffector {
	Kore::vec3 offsetPosition;
	Kore::Quaternion offsetRotation;
	int boneIndex;
	int trackerIndex = -1;
	int ikMode = ikMode; // 0: JT, 1: JPI, 2: DLS, 3: SVD, 4: DLS with SVD, 5: SDLS, 6: SDLS-Modified
	
	EndEffector(int boneIndex, int mode = 5) : boneIndex(boneIndex), ikMode(mode) {}
	
	void setTrackerIndex(int index) {
		trackerIndex = index;
	}
};

struct DataFile {
	const char* positionDataFilename;
	const float scale;
	
	DataFile(const char* position, float scale = 1.0f) : positionDataFilename(position), scale(scale) {}
};

namespace {
	EndEffector* tracker[] = {
		new EndEffector(2), 	// back
		new EndEffector(17), 	// left-hand // todo: oder 16?
		new EndEffector(27), 	// right-hand // todo: oder 26?
		new EndEffector(6),		// left-foot
		new EndEffector(31), 	// right-foot
	};
	
	DataFile* testFile = new DataFile("positionData_mix.csv", 1.08f);
	DataFile* squatsFile = new DataFile("positionData_squats.csv", 1.08f);
	DataFile* currentFile = squatsFile;
	
	const float nearNull = 0.0001f;
	const int width = 1024;
	const int height = 768;
	const bool eval = false;
	const bool withOrientation = true;
	const float errorMaxPos = 0.01f;
	const float errorMaxRot = 0.01f;
}
