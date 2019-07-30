#pragma once

namespace {
	const int numFiles = 7;
    const char* files[numFiles] = {"walking.csv", "lunges.csv", "squats.csv", "kicks.csv", "yoga1.csv", "yoga2.csv", "yoga3.csv"};
	int currentFile = 0;
	
	const float nearNull = 0.0001f;
	
	bool logRawData = false;
	
	// IK Parameter
	int ikMode = 0;
	float lambda[] =		{ 0.35f,	0.05f,		0.2f,		0.03f,		0.2f,		0.018f	};
	float errorMaxPos[] =	{ 0.01f,	0.1f,		0.001f,		0.01f,		0.001f,		0.01f	};
	float errorMaxRot[] =	{ 0.01f,	0.1f,		0.01f,		0.01f,		0.01f,		0.01f	};
	float maxSteps[] =		{ 10.0f,	100.0f,		20.0f,		10.0f,		20.0f,		60.0f	};
	
}
