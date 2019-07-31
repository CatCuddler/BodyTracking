#pragma once

#include "Logger.h"
#include "kMeans.h"
#include "Markov.h"

enum Yoga {
	Yoga1 = 0, Yoga2 = 1, Yoga3 = 2
};

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
	
	bool recording;
	bool recognizing;
	
	void startRecording(Kore::vec3 hmdPosition, Kore::Quaternion hmdRotation);
	void stopRecording();
	
	void startRecognition(Kore::vec3 hmdPosition, Kore::Quaternion hmdRotation);
	bool stopRecognition(Yoga yogaPos);
	bool stopRecognitionAndIdentify();
	
	bool hmmActive();
	bool hmmRecording();
	bool hmmRecognizing();
	void recordMovement(float lastTime, const char* name, Kore::vec3 position, Kore::Quaternion rotation);
	
};
