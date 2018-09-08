#pragma once

#include "EndEffector.h"

#include "kMeans.h"
#include "Markov.h"

class HMM {
	
private:
	const bool record = false;
	const bool recognition = false;
	
	void init(Kore::vec3 hmdPosition, Kore::Quaternion hmdRotation);
	
public:
	HMM();
	
	bool isRecordingActive();
	bool isRecognitionActive();
	
	bool recording = false;
	bool recognizing = false;
	
	void startRecord(Kore::vec3 hmdPosition, Kore::Quaternion hmdRotation);
	void stopRecord();
	
	void startRecognition(Kore::vec3 hmdPosition, Kore::Quaternion hmdRotation);
	bool stopRecognition();
	
	bool hmmActive();
	void recordMovement(const char* name, Kore::vec3 position, Kore::Quaternion rotation);
	
};
