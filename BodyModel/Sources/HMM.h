#pragma once

//#include "EndEffector.h"

#include "kMeans.h"
#include "Markov.h"
#include "matrix.h"

class HMM {
	
private:
	const char* hmmPath = "../Tracking/";
	const char* hmmName = "Yoga_Krieger";
	
	bool record = false;
	bool recognition = false;
	
public:
	HMM();
	
	//void recordMovement(EndEffector* endEffector);
	
};
