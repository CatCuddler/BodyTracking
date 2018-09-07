#pragma once

#include "EndEffector.h"

#include "kMeans.h"
#include "Markov.h"

class HMM {
	
private:
	bool record = false;
	bool recognition = false;
	
public:
	HMM();
	
	void recordMovement(EndEffector& endEffector, float currentUserHeight);
	
	void startRecognition(EndEffector& hmd);
	bool stopRecognition();
	
};
