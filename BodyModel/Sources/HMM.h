#pragma once

#include "Logger.h"
#include "kMeans.h"
#include "Markov.h"

class HMM {
	
private:
	// Either record or recognition can be true
	const bool record = false;
	const bool recognition = false;
	
	Logger& logger;
	
	void init(Kore::vec3 hmdPosition, Kore::Quaternion hmdRotation);
	
public:
	HMM(Logger& logger);
	
	bool isRecordingActive();
	bool isRecognitionActive();
	
	bool recording = false;
	bool recognizing = false;
	
	void startRecording(Kore::vec3 hmdPosition, Kore::Quaternion hmdRotation);
	void stopRecording();
	
	void startRecognition(Kore::vec3 hmdPosition, Kore::Quaternion hmdRotation);
	bool stopRecognition();
	
	bool hmmActive();
	void recordMovement(float lastTime, const char* name, Kore::vec3 position, Kore::Quaternion rotation);
	
};
