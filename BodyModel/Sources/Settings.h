#include <Kore/Math/Core.h>

#pragma once

namespace {
	const int numFiles = 7;
    const char* files[numFiles] = {"walking.csv", "lunges.csv", "squats.csv", "kicks.csv", "yoga1.csv", "yoga2.csv", "yoga3.csv"};
	int currentFile = 0;
	
	const float nearNull = 0.000001f;

	const int numTrackers = 3;
	const bool simpleIK = true; // Simple IK uses only 6 sensors (ignoring forearms)

	/*const int numTrackers = 5;
	const bool simpleIK = false;*/
	
	// IK Parameter
	int ikMode = 2;
	//							JT = 0		JPI = 1		DLS = 2		SVD = 3		SVD_DLS = 4		SDLS = 5
	float lambda[] 			= { 0.35f,		0.05f,		0.25f,		0.03f,		0.25f,			7.0f / 120.0f * Kore::pi	};
	float errorMaxPos[] 	= { 0.01f,		0.1f,		0.001f,		0.01f,		0.001f,			0.01f	};
	float errorMaxRot[] 	= { 0.01f,		0.1f,		0.01f,		0.01f,		0.01f,			0.01f	};
	float maxIterations[]	= { 10.0f,		100.0f,		20.0f,		10.0f,		20.0f,			60.0f	};
	
}
