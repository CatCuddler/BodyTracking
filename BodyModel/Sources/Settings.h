#pragma once

extern int ikMode;
extern float lambda[];

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
	
	DataFile(const char* position, float scale = 1.0f) : positionDataFilename(position) {}
};

namespace {
	EndEffector* tracker[] = {
		new EndEffector(2), 	// back
		new EndEffector(17), 	// left-hand
		new EndEffector(27), 	// right-hand
		new EndEffector(6),		// left-foot
		new EndEffector(31), 	// right-foot
	};
	
	const char* squats[] = {"squats-1.csv", "squats-2.csv", "squats-3.csv", "squats-4.csv", "squats-5.csv"};
	const char* laufen[] = {"laufen-1.csv", "laufen-2.csv", "laufen-3.csv", "laufen-4.csv", "laufen-5.csv"};
    const char* joggen[] = {"joggen-1.csv", "joggen-2.csv", "joggen-3.csv", "joggen-4.csv"};
    const char* alle[] = {"squats-1.csv", "squats-2.csv", "squats-3.csv", "squats-4.csv", "squats-5.csv", "laufen-1.csv", "laufen-2.csv", "laufen-3.csv", "laufen-4.csv", "laufen-5.csv", "joggen-1.csv", "joggen-2.csv", "joggen-3.csv", "joggen-4.csv"};
	const char** currentGroup = joggen;
	
	const float nearNull = 0.0001f;
	const int width = 1024;
	const int height = 768;
	const bool withOrientation = true;
	const float errorMaxPos = 0.01f;
	const float errorMaxRot = 0.01f;
    
    // eval
    const bool eval = true;
    float* evalValue = lambda;
    float evalStep = 0.05f;
    int evalSteps = 21;
    int evalFilesInGroup = 4;
}
