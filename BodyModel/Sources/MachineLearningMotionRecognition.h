#pragma once

#include "Logger.h"
#include "kMeans.h"
#include "Markov.h"
#include <Kore/Input/Keyboard.h>

class MachineLearningMotionRecognition {




private:
	// use this setting to disable the MotionRecognizer or to switch between recording and recognizing movements
	enum MotionRecognitionMode { recordMovements, recognizeMovements, off };
	const MotionRecognitionMode operatingMode = recordMovements;

	// ID of the currently active user, to be used for data differentiation after recording movements
	const string currentTestSubjectID = "TestUser";

	// the file name will be "user + task + session ID (incremented during runtime for each task) + optionalFileTag
	// if you need to restart a recording session, consider incrementing the recordingID from the start, or to add an optionalFileTag
	int sessionID = 0;
	const string optionalFileTag = "";


	Logger& logger;

	void startRecording(bool calibratedAvatar);
	void stopRecording();

	void startRecognition();
	bool stopRecognition();

	bool isCurrentlyRecording();
	bool isCurrentlyRecognizing();



public:

	MachineLearningMotionRecognition(Logger& logger);

	void processKeyDown(Kore::KeyCode code, bool fullyCalibratedAvatar);

	bool isActive();
	bool isProcessingMovementData();
	bool isRecordingMovementData();
	void processMovementData(const char* tag, Kore::vec3 calPos, Kore::Quaternion calRot, Kore::vec3 angVel, Kore::vec3 linVel, float scale, double time);

};