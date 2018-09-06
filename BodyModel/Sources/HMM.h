#pragma once

#include <vector>

class HMM {
	
private:
	const char* hmmPath = "../Tracking/";
	const char* hmmName = "Yoga_Krieger";
	
	// Initial tracked position as base for rotation of any futher data points
	double startX;
	double startZ;
	double startRotCos;
	double startRotSin;
	double transitionX;
	double transitionY;
	double transitionZ;
	int dataPointNumber; // x-th point of data collected in current recording/recognition
	
	// Vector of data points logged in real time movement recognition
	//std::vector<std::vector<Point>> recognitionPoints(6);
	
public:
	HMM();
	
};
