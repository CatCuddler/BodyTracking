#pragma once

extern int ikMode;
extern float lambda[];

struct DataFile {
	const char* positionDataFilename;
	
	DataFile(const char* position, float scale = 1.0f) : positionDataFilename(position) {}
};

namespace {
	const char* squats[] = {"squats-1.csv", "squats-2.csv", "squats-3.csv", "squats-4.csv", "squats-5.csv"};
	const char* laufen[] = {"laufen-1.csv", "laufen-2.csv", "laufen-3.csv", "laufen-4.csv", "laufen-5.csv"};
    const char* joggen[] = {"joggen-1.csv", "joggen-2.csv", "joggen-3.csv", "joggen-4.csv"};
    const char* alle[] = {"squats-1.csv", "squats-2.csv", "squats-3.csv", "squats-4.csv", "squats-5.csv", "laufen-1.csv", "laufen-2.csv", "laufen-3.csv", "laufen-4.csv", "laufen-5.csv", "joggen-1.csv", "joggen-2.csv", "joggen-3.csv", "joggen-4.csv"};
	const char** currentGroup = alle;
	
	const float nearNull = 0.0001f;
	const int width = 1024;
	const int height = 768;

	const bool renderRoom = true;
	const bool renderTrackerAndController = false;
	const bool renderAxisForEndEffector = false;
	
	const bool withOrientation = true;
	const float errorMaxPos = 0.01f;
	const float errorMaxRot = 0.01f;
	
#ifdef KORE_STEAMVR
	bool logData = false;
#endif
	
    // Evaluation values
    const bool eval = false;
    float* evalValue = lambda;
    float evalStep = 0.05f;
    int evalSteps = 21;
    int evalFilesInGroup = 4;
}
