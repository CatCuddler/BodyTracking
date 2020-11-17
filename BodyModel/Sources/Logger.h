#pragma once

#include "EndEffector.h"

#include <Kore/Math/Quaternion.h>

#include <fstream>

class Logger {
	
private:
	// Input and output file to for raw data
	std::fstream logDataReader;
	std::ofstream logDataWriter;
	
	// Output file to save data for hmm
	std::ofstream hmmWriter;
	std::ofstream hmmAnalysisWriter;
	
	// Output file to save data for evaluation
	std::fstream evaluationDataOutputFile;
	
public:
	Logger();
	~Logger();
	
	void startLogger(const char* filename);
	void endLogger();
	void saveData(const char* tag, Kore::vec3 rawPos, Kore::Quaternion rawRot, float scale);
	
	void saveEvaluationData(const char* filename, const float* iterations, float meanErrorPos, float stdErrorPos, float meanErrorRot, float stdErrorRot, const float* time, const float* timeIteration, float reached, float stucked, const float* errorHead, const float* errorHip, const float* errorLeftHand, const float* errorLeftForeArm, const float* errorRightHand, const float* errorRightForeArm, const float* errorLeftFoot, const float* errorRightFoot, const float* errorLeftKnee, const float* errorRightKnee);
	void endEvaluationLogger();
	
	// HMM
	void startHMMLogger(const char* filename, int num);
	void endHMMLogger();
	void saveHMMData(const char* tag, float lastTime, Kore::vec3 pos, Kore::Quaternion rot);
	void analyseHMM(const char* filename, double probability, bool newLine);
	
	bool readData(const int numOfEndEffectors, const char* filename, Kore::vec3* rawPos, Kore::Quaternion* rawRot, EndEffectorIndices indices[], float& scale);
};
