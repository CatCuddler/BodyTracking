#pragma once

#include "Logger.h"
#include "kMeans.h"
#include "Markov.h"
#include <Kore/Input/Keyboard.h>
#include <iostream>
#include <jni.h>

// Responsible for recording data produced by HTC Vive sensors,
// and predicting the currently performed exercise through the Weka machine learning framework
class MachineLearningMotionRecognition {

private:

	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
	//////////////////                                //////////////////
	//////////////////            Settings            //////////////////
	//////////////////                                //////////////////
	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////


	// use this setting to disable the MotionRecognizer 
	// or to switch between recording and recognizing movements
	enum MotionRecognitionMode { RecordMovements, RecognizeMovements, Off };
	const MotionRecognitionMode operatingMode = Off;

	// ID of the currently active user, to be used for 
	// data differentiation after recording movements
	const string currentTestSubjectID = "TestSubjectName";

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


	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
	/////////////////                                 //////////////////
	/////////////////         End of Settings         //////////////////
	/////////////////                                 //////////////////
	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////


	// responsible for recording incoming movement data
	Logger& logger;

	// Start recording incoming movement data
	void startRecording(bool calibratedAvatar);
	// Stop recording incoming movement data
	void stopRecording();

	// Start to use incoming movement data for motion recognition
	void startRecognition();
	// Stop using incoming movement data for motion recognition
	void stopRecognition();
	// Switch between using incoming movement data for motion recognition, or not
	void toggleRecognition();

	// Whether incoming movement data is currently
	// being recorded
	bool isCurrentlyRecording();

	// Whether incoming movement data is currently 
	// used for motion recognition
	bool isCurrentlyRecognizing();

	// Create a Java Virtual Machine, 
	// and load and initialize the Java class we need to interact with
	void initializeJavaNativeInterface();


public:

	// Create and initialize
	MachineLearningMotionRecognition(Logger& logger);

	// Forward key presses to allow local handling of inputs
	// E.g., trigger recording of a specific task, or start recognition
	void processKeyDown(Kore::KeyCode code, bool fullyCalibratedAvatar);

	// Whether either recording or recognition has been requested
	// Will only return false if the operating mode is set to "Off"
	bool isActive();

	// Whether movement data is currently of any interest 
	// E.g. for recording or recognition
	bool isProcessingMovementData();

	// Whether movement data is currently being recorded
	// Only returns true if data is actually being recorded, not if it has only been requested
	bool isRecordingMovementData();

	// Forward sensor readings for local handling
	// E.g., recording or recognizing the data
	void processMovementData(
		const char* tag,
		Kore::vec3 rawPos, Kore::vec3 desPos, Kore::vec3 finalPos,
		Kore::Quaternion rawRot, Kore::Quaternion desRot, Kore::Quaternion finalRot,
		Kore::vec3 rawAngVel, Kore::Quaternion desAngVel,
		Kore::vec3 rawLinVel, Kore::vec3 desLinVel,
		float scale, double time);

};