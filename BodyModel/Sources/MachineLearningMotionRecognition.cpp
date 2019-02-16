#include "pch.h"
#include "MachineLearningMotionRecognition.h"

#include <Kore/Log.h>
#include <Kore/Input/Keyboard.h>

#include <Kore/Audio1/Audio.h>
#include <Kore/Audio1/Sound.h>

#include <algorithm>

#include <iostream>
#include <jni.h>
#include <windows.h>


namespace {

	const string safePath = "../../MachineLearningMotionRecognition/Recordings/";

	// ID for the task currently / last recorded, and the task that is about to be recorded 
	string taskCurrentlyRecording = "TestTask";
	string taskNextToRecord = "nextTestTask";


	// whether recording or recognizing is currently in progress
	bool currentlyRecording = false;
	bool currentlyRecognizing = false;

	// Audio cues
	Kore::Sound* startRecordingSound;
	Kore::Sound* stopRecordingSound;
	Kore::Sound* wrongSound;
	Kore::Sound* joggingSound;
	Kore::Sound* kickSound;
	Kore::Sound* kickPunchSound;
	Kore::Sound* lateralBoundingSound;
	Kore::Sound* lungesSound;
	Kore::Sound* punchSound;
	Kore::Sound* sittingSound;
	Kore::Sound* squatsSound;
	Kore::Sound* standingSound;
	Kore::Sound* walkingSound;

	// Weka access through the Java Native Interface JNI
	JavaVM *java_VirtualMachine;				// Pointer to the JVM (Java Virtual Machine)
	JNIEnv *java_JNI;				// Pointer to native interface
	jobject java_WekaObject;
	jmethodID java_addDataPointToClassifier;
}

MachineLearningMotionRecognition::MachineLearningMotionRecognition(Logger& logger) : logger(logger) {

	// Sound initiation
	startRecordingSound = new Kore::Sound("sound/start.wav");
	stopRecordingSound = new Kore::Sound("sound/stop.wav");
	wrongSound = new Kore::Sound("sound/wrong.wav");
	joggingSound = new Kore::Sound("sound/mlmr/jogging.wav");
	kickSound = new Kore::Sound("sound/mlmr/kick.wav");
	kickPunchSound = new Kore::Sound("sound/mlmr/kickPunch.wav");
	lateralBoundingSound = new Kore::Sound("sound/mlmr/bounding.wav");
	lungesSound = new Kore::Sound("sound/mlmr/lunges.wav");
	punchSound = new Kore::Sound("sound/mlmr/punch.wav");
	sittingSound = new Kore::Sound("sound/mlmr/sitting.wav");
	squatsSound = new Kore::Sound("sound/mlmr/squats.wav");
	standingSound = new Kore::Sound("sound/mlmr/standing.wav");
	walkingSound = new Kore::Sound("sound/mlmr/walking.wav");


	// state debugging information
	if (operatingMode == RecordMovements) {
		Kore::log(Kore::LogLevel::Info, "Motion Recognition ready to record data for user: \n   %s", currentTestSubjectID.c_str());
	}
	else if (operatingMode == RecognizeMovements) {
		Kore::log(Kore::LogLevel::Info, "Motion Recognition ready to recognize incoming movements");
		initializeJavaNativeInterface();
	}
}

void outputClassifierResultFromWeka(JNIEnv*env, jobject o, jstring jStringResult) {

	// convert and print result string
	const char* charResult = (*env).GetStringUTFChars(jStringResult, 0);
	Kore::log(Kore::LogLevel::Info, "%s", charResult);
	
	
	if (strcmp(charResult, "jogging") == 0) {
		Kore::Audio1::play(joggingSound);
	} 
	else if (strcmp(charResult, "kick") == 0) {
		Kore::Audio1::play(kickSound);
	}
	else if (strcmp(charResult, "kickPunch") == 0) {
		Kore::Audio1::play(kickPunchSound);
	}
	else if (strcmp(charResult, "lateralBounding") == 0) {
		Kore::Audio1::play(lateralBoundingSound);
	}
	else if (strcmp(charResult, "lunges") == 0) {
		Kore::Audio1::play(lungesSound);
	}
	else if (strcmp(charResult, "punch") == 0) {
		Kore::Audio1::play(punchSound);
	}
	else if (strcmp(charResult, "sitting") == 0) {
		Kore::Audio1::play(sittingSound);
	}
	else if (strcmp(charResult, "squats") == 0) {
		Kore::Audio1::play(squatsSound);
	}
	else if (strcmp(charResult, "standing") == 0) {
		Kore::Audio1::play(standingSound);
	}
	else if (strcmp(charResult, "walking") == 0) {
		Kore::Audio1::play(walkingSound);
	}
	

	//release the string to	avoid memory leak
	(*env).ReleaseStringUTFChars(jStringResult, charResult);

}

void MachineLearningMotionRecognition::initializeJavaNativeInterface() {

	// type signature reference for method construction:
	// https://docs.oracle.com/javase/1.5.0/docs/guide/jni/spec/types.html#wp276
	// JNI setup example:
	// https://www.codeproject.com/Articles/993067/Calling-Java-from-Cplusplus-with-JNI


	//==================== prepare loading of Java VM ============================

	JavaVMInitArgs vm_args;                        // Initialization arguments
	JavaVMOption* options = new JavaVMOption[1];   // JVM invocation options
	// where to find the java .class or .jar to load
	// name the folder for single class files, name the jar (including .jar) for classes within a jar
	options[0].optionString =
		"-Djava.class.path=../../MachineLearningMotionRecognition/Weka/WekaMotionRecognitionForCpp-1.0-jar-with-dependencies.jar";
	vm_args.version = JNI_VERSION_1_6;             // minimum Java version
	vm_args.nOptions = 1;                          // number of options
	vm_args.options = options;
	vm_args.ignoreUnrecognized = false;     // invalid options make the JVM init fail

	//================= load and initialize Java VM and JNI interface ===============

	// There WILL be an exception caused by a JVM internal bugfix for certain old operating systems
	// It is handled by the JVM as well and can be ignored
	Kore::log(Kore::LogLevel::Info,
		"The following exception is produced and handled by the Java Virtual Machine, to test for an old OS bug. It can be ignored:");
	// actually load the JVM, causing the exception
	jint rc = JNI_CreateJavaVM(&java_VirtualMachine, (void**)&java_JNI, &vm_args);
	delete options;    // we then no longer need the initialisation options. 
	Kore::log(Kore::LogLevel::Info,
		"The previous exception is produced and handled by the Java Virtual Machine, to test for an old OS bug. It can be ignored.");
	//========================= output error or success  ==============================
	// if process interuped before error is returned, it is often because jvm.dll can't be found,
	// e.g. because its directory is not in the PATH system environment variable

	if (rc != JNI_OK) {
		if (rc == JNI_EVERSION)
			Kore::log(Kore::LogLevel::Error, "JNI ERROR: JVM is oudated and doesn't meet requirements");
		else if (rc == JNI_ENOMEM)
			Kore::log(Kore::LogLevel::Error, "JNI ERROR: not enough memory for JVM");
		else if (rc == JNI_EINVAL)
			Kore::log(Kore::LogLevel::Error, "JNI ERROR: invalid ragument for launching JVM");
		else if (rc == JNI_EEXIST)
			Kore::log(Kore::LogLevel::Error, "JNI ERROR: the process can only launch one JVM and not more");
		else
			Kore::log(Kore::LogLevel::Error, "JNI ERROR:  could not create the JVM instance (error code %i)", rc);
		exit(EXIT_FAILURE);
	}
	else {
		Kore::log(Kore::LogLevel::Info, "JVM loaded without errors");
	}

	// GetVersion returns the major version number in the higher 16 bits and the minor version number in the lower 16 bits
	jint nativeMethodInterfaceVersion = java_JNI->GetVersion();
	int nmiMajorVersion = (int)(nativeMethodInterfaceVersion >> 16);
	int nmiMinorVersion = (int)(nativeMethodInterfaceVersion & 0x0f);
	Kore::log(Kore::LogLevel::Info,
		"Loaded Java Virtual Machine for Weka motion recognition through Java Native Interface, version %i.%i",
		nmiMajorVersion, nmiMinorVersion);


	// try to find the class
	jclass java_WekaClass = java_JNI->FindClass("com/romanuhlig/weka/lifeClassification/CppDataClassifier");


	if (java_WekaClass == nullptr) {
		Kore::log(Kore::LogLevel::Error, "JNI ERROR: class not found!");
	}
	else {
		// if class found, continue
		Kore::log(Kore::LogLevel::Info, "Java class found");

		// Register native output method in Java class
		JNINativeMethod methods[]
		{ { "outputClassifierResultToCpp", "(Ljava/lang/String;)V", (void *)&outputClassifierResultFromWeka } };  // mapping table

		if (java_JNI->RegisterNatives(java_WekaClass, methods, 1) < 0) {                        // register it
			if (java_JNI->ExceptionOccurred())                                        // verify if it's ok
				Kore::log(Kore::LogLevel::Error, "JNI ERROR: exception when registreing naives!");
			else
				Kore::log(Kore::LogLevel::Error, "JNI ERROR: problem when registreing naives!");
		}

		// create the java object
		// find the object constructor
		jmethodID java_WekaClassConstructor = java_JNI->GetMethodID(java_WekaClass, "<init>", "()V");
		if (java_WekaClassConstructor == nullptr) {
			Kore::log(Kore::LogLevel::Error, "JNI ERROR: constructor not found!");
		}
		else {
			// create the actual java object
			java_WekaObject = java_JNI->NewObject(java_WekaClass, java_WekaClassConstructor);
			Kore::log(Kore::LogLevel::Info, "JNI: Object succesfully constructed");

			if (java_WekaObject) {
				// get a method from the object
				java_addDataPointToClassifier = java_JNI->GetMethodID(java_WekaClass, "addFrameData",
					"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;DDDDDDDDDDDDDDD)V");
				if (java_addDataPointToClassifier == nullptr) {
					Kore::log(Kore::LogLevel::Error, "JNI ERROR: No addDataPoint method");
				}
				else {
					//java_JNI->CallVoidMethod(java_WekaObject, java_addDataPointToClassifier,
					//	java_JNI->NewStringUTF("pos"), java_JNI->NewStringUTF("subj"), java_JNI->NewStringUTF("activi"),
					//	(jdouble)1, (jdouble)1, (jdouble)1,
					//	(jdouble)1, (jdouble)1, (jdouble)1, (jdouble)1,
					//	(jdouble)1, (jdouble)1, (jdouble)1,
					//	(jdouble)1, (jdouble)1, (jdouble)1,
					//	(jdouble)1, (jdouble)1);

				}
			}
		}

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
	Kore::log(Kore::Info, "Motion Recognition started");
	currentlyRecognizing = true;
}

void MachineLearningMotionRecognition::stopRecognition() {
	Kore::log(Kore::Info, "Motion Recognition stopped");
	currentlyRecognizing = false;
}

void MachineLearningMotionRecognition::toggleRecognition() {
	if (currentlyRecognizing) {
		stopRecognition();
	}
	else {
		startRecognition();
	}
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

	if (operatingMode == RecordMovements) {
		logger.saveMotionRecognitionData(
			tag, currentTestSubjectID.c_str(), taskCurrentlyRecording.c_str(), 
			rawPos, desPos, finalPos,
			rawRot, desRot, finalRot,
			rawAngVel, desAngVel,
			rawLinVel, desLinVel,
			scale, time);
	}
	else if (operatingMode == RecognizeMovements) {
		if (currentlyRecognizing) {
			if (tag == "rForeArm" || tag == "lLeg") {
				java_JNI->CallVoidMethod(java_WekaObject, java_addDataPointToClassifier,
					java_JNI->NewStringUTF(tag), java_JNI->NewStringUTF(currentTestSubjectID.c_str()), java_JNI->NewStringUTF("unknown"),
					(jdouble)rawPos.x(), (jdouble)rawPos.y(), (jdouble)rawPos.z(),
					(jdouble)rawRot.x, (jdouble)rawRot.y, (jdouble)rawRot.z, (jdouble)rawRot.w,
					(jdouble)rawAngVel.x(), (jdouble)rawAngVel.y(), (jdouble)rawAngVel.z(),
					(jdouble)rawLinVel.x(), (jdouble)rawLinVel.y(), (jdouble)rawLinVel.z(),
					(jdouble)1, (jdouble)time);
			}



			/*
						java_JNI->CallVoidMethod(java_WekaObject, java_addDataPointToClassifier,
							java_JNI->NewStringUTF("pos"), java_JNI->NewStringUTF("subj"), java_JNI->NewStringUTF("activi"),
							(jdouble)calPos.x, (jdouble)calPos.y, (jdouble)calPos.z,
							(jdouble)calRot.x, (jdouble)calRot.y, (jdouble)calRot.z, (jdouble)calRot.w,
							(jdouble)angVel.x, (jdouble)angVel.y, (jdouble)angVel.z,
							(jdouble)linVel.x, (jdouble)linVel.y, (jdouble)linVel.z,
							(jdouble)scale, (jdouble)time);*/
		}
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
		default:
			break;
		}
	}
	else if (operatingMode == RecognizeMovements) {
		switch (code) {
			// end the recording if space is pressed
		case Kore::KeySpace:
			toggleRecognition();
			break;
		default:
			break;
		}
	}
}

