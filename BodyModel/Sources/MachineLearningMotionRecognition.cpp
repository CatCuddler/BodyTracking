#include "pch.h"
#include "MachineLearningMotionRecognition.h"

#include <Kore/Log.h>
#include <Kore/Input/Keyboard.h>

#include <Kore/Audio1/Audio.h>
#include <Kore/Audio1/Sound.h>

#include <algorithm>


namespace {

	const string safePath = "../../MachineLearningMotionRecognition/Recordings/";

	// ID for the task currently / last recorded, and the task that is about to be recorded 
	string taskCurrentlyRecording = "TestTask";
	string taskNextToRecord = "nextTestTask";


	// whether recording or recognizing is currently in progress
	bool currentlyRecording = false;
	bool currentlyRecognizing = false;

	// Audio cues
	// TODO: -replace placeholder sounds with specific sounds for each task, and a proper error sound
	Kore::Sound* startRecordingSound;
	Kore::Sound* stopRecordingSound;
	Kore::Sound* wrongSound;
}

MachineLearningMotionRecognition::MachineLearningMotionRecognition(Logger& logger) : logger(logger) {

	// Sound initiation
	startRecordingSound = new Kore::Sound("sound/start.wav");
	stopRecordingSound = new Kore::Sound("sound/stop.wav");
	wrongSound = new Kore::Sound("sound/wrong.wav");

	// state debugging information
	if (operatingMode == RecordMovements) {
		Kore::log(Kore::LogLevel::Info, "Motion Recognition ready to record data for user: \n   %s", currentTestSubjectID.c_str());
	}
	else if (operatingMode == RecognizeMovements) {
		Kore::log(Kore::LogLevel::Info, "Motion Recognition ready to recognize incoming movements");
	}
}

bool MachineLearningMotionRecognition::isCurrentlyRecording() {
	return (currentlyRecording);
}

bool MachineLearningMotionRecognition::isCurrentlyRecognizing() {
	return (currentlyRecognizing);
}


void MachineLearningMotionRecognition::startRecording(bool fullyCalibratedAvatar) {

	if (!fullyCalibratedAvatar) {
		Kore::log(Kore::LogLevel::Warning,
			"Unable to start !!! \n   Recording cannot start until Avatar is fully calibrated");
		Kore::Audio1::play(wrongSound);
	}
	else if (currentlyRecording) {
		Kore::log(Kore::LogLevel::Warning,
			"Recording already in progress !!! \n   %s can not be recorded \n   You did not stop recording for %s !!!", taskNextToRecord.c_str(), taskCurrentlyRecording.c_str());
		Kore::Audio1::play(wrongSound);
	}
	else {
		currentlyRecording = true;
		taskCurrentlyRecording = taskNextToRecord;
		sessionID++;

		string fileNameString = safePath + currentTestSubjectID + "__" + taskCurrentlyRecording + "__SID" + std::to_string(sessionID) + "_" + optionalFileTag;
		logger.startMotionRecognitionLogger(fileNameString.c_str());

		Kore::log(Kore::LogLevel::Info, "started recording ID %i:   %s   (%s)", sessionID, taskCurrentlyRecording.c_str(), currentTestSubjectID.c_str());
		Kore::Audio1::play(startRecordingSound);
	}

}

void MachineLearningMotionRecognition::stopRecording() {

	if (currentlyRecording) {
		currentlyRecording = false;
		logger.endMotionRecognitionLogger();

		Kore::log(Kore::LogLevel::Info, "recording ID %i stopped:   %s   (%s)", sessionID, taskCurrentlyRecording.c_str(), currentTestSubjectID.c_str());
		Kore::Audio1::play(stopRecordingSound);
	}
	else {
		Kore::log(Kore::LogLevel::Warning, "You tried to stop recording while no recording was in progress !!! \n   Last recording was %s", taskCurrentlyRecording.c_str());
		Kore::Audio1::play(wrongSound);
	}
}

void MachineLearningMotionRecognition::startRecognition() {
	Kore::log(Kore::Error, "Motion Recognition not yet implemented");
	currentlyRecognizing = true;
}

bool MachineLearningMotionRecognition::stopRecognition() {
	Kore::log(Kore::Error, "Motion Recognition not yet implemented");
	currentlyRecognizing = false;
	return false;
}

bool MachineLearningMotionRecognition::isProcessingMovementData() {
	return (currentlyRecording || currentlyRecognizing);
}

bool MachineLearningMotionRecognition::isRecordingMovementData()
{
	return currentlyRecording;
}

void MachineLearningMotionRecognition::processMovementData(
	const char* tag, 
	Kore::vec3 rawPos, Kore::vec3 desPos, Kore::vec3 finalPos,
	Kore::Quaternion rawRot, Kore::Quaternion desRot, Kore::Quaternion finalRot,
	Kore::vec3 rawAngVel, Kore::Quaternion desAngVel,
	Kore::vec3 rawLinVel, Kore::vec3 desLinVel,
	float scale, double time) {

	if (currentlyRecording) {
		logger.saveMotionRecognitionData(
			tag, currentTestSubjectID.c_str(), taskCurrentlyRecording.c_str(), 
			rawPos, desPos, finalPos,
			rawRot, desRot, finalRot,
			rawAngVel, desAngVel,
			rawLinVel, desLinVel,
			scale, time);
	}
}

bool MachineLearningMotionRecognition::isActive() {
	return operatingMode != Off;
}


void MachineLearningMotionRecognition::processKeyDown(Kore::KeyCode code, bool fullyCalibratedAvatar)
{

	if (operatingMode == RecordMovements) {

		switch (code) {
			// end the recording if space is pressed
		case Kore::KeySpace:
			stopRecording();
			break;
			// start a new recording if the corresponding numpad key is pressed
		case Kore::KeyNumpad1:
			taskNextToRecord = task_01;
			startRecording(fullyCalibratedAvatar);
			break;
		case Kore::KeyNumpad2:
			taskNextToRecord = task_02;
			startRecording(fullyCalibratedAvatar);
			break;
		case Kore::KeyNumpad3:
			taskNextToRecord = task_03;
			startRecording(fullyCalibratedAvatar);
			break;
		case Kore::KeyNumpad4:
			taskNextToRecord = task_04;
			startRecording(fullyCalibratedAvatar);
			break;
		case Kore::KeyNumpad5:
			taskNextToRecord = task_05;
			startRecording(fullyCalibratedAvatar);
			break;
		case Kore::KeyNumpad6:
			taskNextToRecord = task_06;
			startRecording(fullyCalibratedAvatar);
			break;
		case Kore::KeyNumpad7:
			taskNextToRecord = task_07;
			startRecording(fullyCalibratedAvatar);
			break;
		case Kore::KeyNumpad8:
			taskNextToRecord = task_08;
			startRecording(fullyCalibratedAvatar);
			break;
		case Kore::KeyNumpad9:
			taskNextToRecord = task_09;
			startRecording(fullyCalibratedAvatar);
			break;
		case Kore::KeyNumpad0:
			taskNextToRecord = task_00;
			startRecording(fullyCalibratedAvatar);
			break;
		default:
			break;
		}
	}
}

