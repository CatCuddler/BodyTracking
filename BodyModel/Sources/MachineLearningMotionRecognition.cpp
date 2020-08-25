#include "pch.h"
#include "MachineLearningMotionRecognition.h"

#include <Kore/Log.h>

#include <algorithm>

#include <string>
#include <iostream>

#ifdef KORE_STEAMVR
#include <jni.h>
#include <windows.h>
#endif

namespace {

	// storage destination for recorded sensor data
	const string safePath = "../../MachineLearningMotionRecognition/Recordings/";

	// ID for the task currently / last recorded, and the task that is about to be recorded
	string taskCurrentlyRecording = "TestTask";
	string taskNextToRecord = "nextTestTask";

	// whether recording or recognizing is currently in progress
	bool currentlyRecording = false;
	bool currentlyRecognizing = false;

	const char* lastRecognizedActivity = "Unknown";

	bool weka_head = false;
	bool weka_hip = false;
	bool weka_leftArm = false;
	bool weka_rightArm = false;
	bool weka_leftLeg = false;
	bool weka_rightLeg = false;

	// Weka access through the Java Native Interface JNI
#ifdef KORE_STEAMVR
	JavaVM *java_VirtualMachine;				// Pointer to the JVM (Java Virtual Machine)
	JNIEnv *java_JNI;							// Pointer to native interface
	jobject java_WekaObject;					// The Java object we want to communicate with
	jmethodID java_addDataPointToClassifier;	// The Java function we want to call
	jmethodID java_recognize;					// The Java function we want to call
	jmethodID java_getFeedback;					// The Java function we want to call
	
	// Called from the Java environment, to inform us of the Weka exercise prediction
	void outputClassifierResultFromWeka(JNIEnv* env, jobject o, jstring jStringResult) {

		// convert and print result string
		const char* charResult = (*env).GetStringUTFChars(jStringResult, 0);
		Kore::log(Kore::LogLevel::Info, "Weka detected %s", charResult);

		lastRecognizedActivity = charResult;

		//release the string to	avoid memory leak
		(*env).ReleaseStringUTFChars(jStringResult, charResult);
	}

	// Create a Java Virtual Machine and load and initialize the Java class we need to interact with
	void initializeJavaNativeInterface() {
		// type signature reference for method construction:
		// https://docs.oracle.com/javase/1.5.0/docs/guide/jni/spec/types.html#wp276
		// JNI setup example:
		// https://www.codeproject.com/Articles/993067/Calling-Java-from-Cplusplus-with-JNI


		//==================== prepare loading of Java VM ============================

		JavaVMInitArgs vm_args;							// Initialization arguments
		JavaVMOption* options = new JavaVMOption[1];	// JVM invocation options
		// where to find the java .class or .jar to load
		// name the folder for single class files, name the jar (including .jar) for classes within a jar
		options[0].optionString = "-Djava.class.path=../../MachineLearningMotionRecognition/Weka/WekaMotionRecognitionForCpp.jar";
		vm_args.version = JNI_VERSION_1_6;				// minimum Java version
		vm_args.nOptions = 1;							// number of options
		vm_args.options = options;
		vm_args.ignoreUnrecognized = false;				// invalid options make the JVM init fail

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
			{ { "outputClassifierResultToCpp", "(Ljava/lang/String;)V", (void*)&outputClassifierResultFromWeka } };  // mapping table

			if (java_JNI->RegisterNatives(java_WekaClass, methods, 1) < 0) {              // register it
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

					java_recognize = java_JNI->GetMethodID(java_WekaClass, "recognizeLastMovement", "()V");
					if (java_recognize == nullptr) {
						Kore::log(Kore::LogLevel::Error, "JNI ERROR: No recognizeLastMovement method");
					}

					java_getFeedback = java_JNI->GetMethodID(java_WekaClass, "getFeedbackForEachSensor", "()V");
					if (java_recognize == nullptr) {
						Kore::log(Kore::LogLevel::Error, "JNI ERROR: No getFeedbackForEachSensor method");
					}
				}
			}
		}
	}

#endif
}


MachineLearningMotionRecognition::MachineLearningMotionRecognition(Logger& logger) : logger(logger) {

	// log debugging information
	if (operatingMode == RecordMovements) {
		Kore::log(Kore::LogLevel::Info, "Motion Recognition ready to record data for user: \n   %s", currentTestSubjectID.c_str());
	}
	else if (operatingMode == RecognizeMovements) {
		Kore::log(Kore::LogLevel::Info, "Motion Recognition ready to recognize incoming movements");
		initializeJavaNativeInterface();
	}
}

bool MachineLearningMotionRecognition::isCurrentlyRecording() {
	return currentlyRecording;
}


bool MachineLearningMotionRecognition::isCurrentlyRecognizing() {
	return currentlyRecognizing;
}

void MachineLearningMotionRecognition::startRecording(bool fullyCalibratedAvatar) {

	// do not start recording, if avatar is not fully calibrated
	if (!fullyCalibratedAvatar) {
		Kore::log(Kore::LogLevel::Warning,
			"Unable to start !!! \n   Recording cannot start until Avatar is fully calibrated");
	}
	// do not start a new recording, if a previous recording is still in progress
	else if (currentlyRecording) {
		Kore::log(Kore::LogLevel::Warning,
			"Recording already in progress !!! \n   %s can not be recorded \n   You did not stop recording for %s !!!",
			taskNextToRecord.c_str(), taskCurrentlyRecording.c_str());
	}
	// otherwise, go for it
	else {

		// update recording state
		currentlyRecording = true;
		taskCurrentlyRecording = taskNextToRecord;
		sessionID++;

		// determine file name and start recording
		string fileNameString = safePath + currentTestSubjectID + "__" + taskCurrentlyRecording
			+ "__SID" + std::to_string(sessionID) + "_" + optionalFileTag;
		logger.startMotionRecognitionLogger(fileNameString.c_str());

		// log start of new recording, and notify user via sound
		Kore::log(Kore::LogLevel::Info, "started recording ID %i:   %s   (%s)",
			sessionID, taskCurrentlyRecording.c_str(), currentTestSubjectID.c_str());
	}

}

void MachineLearningMotionRecognition::stopRecording() {

	// can only stop recording if one is actually in progress
	if (currentlyRecording) {

		// update recording state
		currentlyRecording = false;
		logger.endMotionRecognitionLogger();

		// log end of recording, and notify user via sound
		Kore::log(Kore::LogLevel::Info, "recording ID %i stopped:   %s   (%s)",
			sessionID, taskCurrentlyRecording.c_str(), currentTestSubjectID.c_str());
	}
	// if user tried to stop a recording that was not actually in progress,
	// let them know that they made a mistake via log and sound
	// (user needs to know they might have forgotten to start previous recording)
	else {
		Kore::log(Kore::LogLevel::Warning,
			"You tried to stop recording while no recording was in progress !!! \n   Last recording was %s",
			taskCurrentlyRecording.c_str());
	}
}

void MachineLearningMotionRecognition::startRecognition() {
	currentlyRecognizing = true;
}

void MachineLearningMotionRecognition::stopRecognition() {
	currentlyRecognizing = false;
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

	// when recording movements, forward the data to the logger
	if (operatingMode == RecordMovements) {
		logger.saveMotionRecognitionData(
			tag, currentTestSubjectID.c_str(), taskCurrentlyRecording.c_str(),
			rawPos, desPos, finalPos,
			rawRot, desRot, finalRot,
			rawAngVel, desAngVel,
			rawLinVel, desLinVel,
			scale, time);
	}
	// when recognizing movements, forward the data to the (Java-based) Weka Helper
	else if (operatingMode == RecognizeMovements) {
		if (currentlyRecognizing) {
			
			#ifdef KORE_STEAMVR
			if (tag == "head" || tag == "hip" || tag == "lHand" || tag == "rHand" || tag == "lFoot" || tag == "rFoot") {
				java_JNI->CallVoidMethod(java_WekaObject, java_addDataPointToClassifier,
					java_JNI->NewStringUTF(tag), java_JNI->NewStringUTF(currentTestSubjectID.c_str()), java_JNI->NewStringUTF("unknown"),
					//(jdouble)rawPos.x(), (jdouble)rawPos.y(), (jdouble)rawPos.z(),
					//(jdouble)rawRot.x, (jdouble)rawRot.y, (jdouble)rawRot.z, (jdouble)rawRot.w,
					(jdouble)finalPos.x(), (jdouble)finalPos.y(), (jdouble)finalPos.z(),
					(jdouble)finalRot.x, (jdouble)finalRot.y, (jdouble)finalRot.z, (jdouble)finalRot.w,
					(jdouble)rawAngVel.x(), (jdouble)rawAngVel.y(), (jdouble)rawAngVel.z(),
					(jdouble)rawLinVel.x(), (jdouble)rawLinVel.y(), (jdouble)rawLinVel.z(),
					(jdouble)1, (jdouble)time);
			}
			#endif
		}
	}
}

const char* MachineLearningMotionRecognition::getRecognizedActivity() {
	#ifdef KORE_STEAMVR
	if (operatingMode == RecognizeMovements) {
		if (currentlyRecognizing) {
			java_JNI->CallVoidMethod(java_WekaObject, java_recognize);
		}
	}
	#endif
	return lastRecognizedActivity;
}

void MachineLearningMotionRecognition::getFeedback(bool &head, bool &hip, bool &leftArm, bool &rightArm, bool &leftLeg, bool &rightLeg) {
	
	#ifdef KORE_STEAMVR
	if (operatingMode == RecognizeMovements) {
		if (currentlyRecognizing) {
			java_JNI->CallVoidMethod(java_WekaObject, java_getFeedback);
		}
	}
	#endif
	
	head = weka_head;
	hip = weka_hip;
	leftArm = weka_leftArm;
	rightArm = weka_rightArm;
	leftLeg = weka_leftLeg;
	rightLeg = weka_rightLeg;
}

bool MachineLearningMotionRecognition::isActive() {
	return operatingMode != Off;
}


void MachineLearningMotionRecognition::processKeyDown(Kore::KeyCode code, bool fullyCalibratedAvatar) {

	// when recording, start or stop recordings
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
}