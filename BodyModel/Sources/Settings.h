#pragma once

extern int ikMode;
extern float lambda[];

struct DataFile {
	const char* positionDataFilename;
	
	DataFile(const char* position, float scale = 1.0f) : positionDataFilename(position) {}
};

namespace {
	// TODO: Record new data (raw data)
	const int numFiles = 9;
    const char* files[numFiles] = {"positionData_1535958687.csv", "positionData_1535958747.csv", "positionData_1535958762.csv"};
	
	const float nearNull = 0.0001f;
	const int width = 1024;
	const int height = 768;

	const bool renderRoom = true;
	const bool renderTrackerAndController = true;
	const bool renderAxisForEndEffector = false;
	
	const bool withOrientation = true;
	const float errorMaxPos = 0.01f;
	const float errorMaxRot = 0.01f;
	
	bool logData = false;
	
    // Evaluation values
    const bool eval = false;
    float* evalValue = lambda;
    float evalStep = 0.05f;
    int evalSteps = 21;
    int evalFilesInGroup = 4;
}
