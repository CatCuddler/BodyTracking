#pragma once

namespace {
	const int numFiles = 2;
    const char* files[numFiles] = {"scale_test.csv", "walking.csv"};
	int currentFile = 0;
	
	const float nearNull = 0.0001f;
	
	bool logRawData = false;
	
	// IK Parameter
	int ikMode = 0;
	float lambda[] =		{ 0.35f,	0.05f,		0.2f,		0.03f,		0.2f,		0.018f	};
	float errorMaxPos[] =	{ 0.01f,	0.1f,		0.001f,		0.01f,		0.001f,		0.01f	};
	float errorMaxRot[] =	{ 0.01f,	0.1f,		0.01f,		0.01f,		0.01f,		0.01f	};
	float maxSteps[] =		{ 10.0f,	100.0f,		20.0f,		10.0f,		20.0f,		60.0f	};
    
    // Evaluation values
    const bool eval = false;
    float* evalValue = lambda;
	float evalInitValue[] = { 0.35f,	0.05f,		0.2f,		0.03f,		0.2f,		0.018f };
	float evalStep = 0.1f;
	int evalSteps = 10;
    int evalFilesInGroup = numFiles;
	int evalMinIk = 0;
	int evalMaxIk = 5;
	int evalStepsInit = evalSteps;
	
}
