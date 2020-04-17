#pragma once

#include "Logger.h"
#include "kMeans.h"
#include "Markov.h"

enum Yoga {
	Yoga0, Yoga1, Yoga2, Unknown,
};

enum Feedback {
	Head, Hip, LeftArm, RightArm, LeftLeg, RightLeg, CheckMark, CrossMark,
};

// Yoga0 - hands up
// Yoga1 - both hands parallel to the floor
// Yoga2 - hands on the knee

class HMM {
	
private:
	// Either record or recognition can be true
	const bool record = false;
	const bool recognition = true;
	
	Logger& logger;
	
	void init(Kore::vec3 hmdPosition, Kore::Quaternion hmdRotation);
	
	bool stopRecognition(const char* path);
	
public:
	HMM(Logger& logger);
	
	bool isRecordingActive();
	bool isRecognitionActive();
	
	void startRecording(Kore::vec3 hmdPosition, Kore::Quaternion hmdRotation);
	void stopRecording();
	
	void startRecognition(Kore::vec3 hmdPosition, Kore::Quaternion hmdRotation);
	bool stopRecognitionAndIdentify(Yoga yogaPos);
	bool stopRecognitionAndIdentify();
	
	void recordMovement(float lastTime, const char* name, Kore::vec3 position, Kore::Quaternion rotation);
	
	void getFeedback(bool &head, bool &hip, bool &leftArm, bool &rightArm, bool &leftLeg, bool &rightLeg);
	
};
