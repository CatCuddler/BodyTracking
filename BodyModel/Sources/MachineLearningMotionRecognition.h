#pragma once

#include "Logger.h"
#include "kMeans.h"
#include "Markov.h"
#include <Kore/Input/Keyboard.h>

class MachineLearningMotionRecognition {




private:
	// use this setting to disable the MotionRecognizer 
	// or to switch between recording and recognizing movements
	enum MotionRecognitionMode { RecordMovements, RecognizeMovements, Off };
	const MotionRecognitionMode operatingMode = RecordMovements;

	// ID of the currently active user, to be used for 
	// data differentiation after recording movements
	const string currentTestSubjectID = "ExampleUserName";

	// the file name will be "user + task + session ID + optionalFileTag"
	// The sessionID is incremented during runtime for each task. If you 
	// need to restart a recording session, consider incrementing the 
	// recordingID from the start, or to add an optionalFileTag
	int sessionID = 0;
	const string optionalFileTag = "";

	// task 00 is recorded with numpad 0, 01 with 1...
	// stop recording with space bar
	const string task_00 = "walking";
	const string task_01 = "jogging";
	const string task_02 = "squats";
	const string task_03 = "lunges";
	const string task_04 = "standing";
	const string task_05 = "sitting";
	const string task_06 = "lateralBounding";
	const string task_07 = "kick";
	const string task_08 = "punch";
	const string task_09 = "kickPunch";


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
	void processMovementData(
		const char* tag,
		Kore::vec3 rawPos, Kore::vec3 desPos, Kore::vec3 finalPos,
		Kore::Quaternion rawRot, Kore::Quaternion desRot, Kore::Quaternion finalRot,
		Kore::vec3 rawAngVel, Kore::Quaternion desAngVel,
		Kore::vec3 rawLinVel, Kore::vec3 desLinVel,
		float scale, double time);

};