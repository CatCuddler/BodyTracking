#pragma once

enum IKMode {
	JT = 0, JPI = 1, DLS = 2, SVD = 3, SVD_DLS = 4, SDLS = 5
};

namespace {
	const int numFiles = 7;
    const char* files[numFiles] = {"walking.csv", "lunges.csv", "squats.csv", "kicks.csv", "yoga1.csv", "yoga2.csv", "yoga3.csv"};
	int currentFile = 0;
	
	const float nearNull = 0.0001f;
	
	bool logRawData = false;
	int ikMode = DLS;

	// Optimized IK Parameter
	//										JT = 0		JPI = 1		DLS = 2		SVD = 3		SVD_DLS = 4	SDLS = 5
	const float optimalLambda[6] =			{ 0.35f,	0.05f,		0.2f,		0.03f,		0.2f,		0.018f	};
	const float optimalErrorMaxPos[6] =		{ 0.01f,	0.1f,		0.001f,		0.01f,		0.001f,		0.01f	};
	const float optimalErrorMaxRot[6] =		{ 0.01f,	0.1f,		0.01f,		0.01f,		0.01f,		0.01f	};
	const float optimalMaxIterations[6] =	{ 10.0f,	100.0f,		20.0f,		10.0f,		20.0f,		60.0f	};
    
    // Evaluation values
    const bool eval = false;
	const float evalInitValue[6] =	{ 0.05f,	1.0f,		0.0f,		0.01f,		0.0f,		0.002f };
	const float evalStep[6] = 		{ 0.05f,	0.0,		0.05f,		0.01f,		0.05f,		0.002f };
	const float evalMaxValue[6] = 	{ 1.0f,		1.0f,		1.5f,		0.25f,		0.15f,		0.0052f };
    const int evalFilesInGroup = numFiles;
	const int evalMinIk = 0;
	const int evalMaxIk = 5;
}
