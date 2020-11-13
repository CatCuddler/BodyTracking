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
	
	void saveEvaluationData(const char* filename, const float* iterations, const float* errorPos, const float* errorRot, const float* time, const float* timeIteration, bool reached, bool stucked, float errorPosHead, float errorPosHip, float errorPosLeftHand, float errorPosLeftForeArm, float errorPosRightHand, float errorPosRightForeArm, float errorPosLeftFoot, float errorPosRightFoot, float errorPosLeftKnee, float errorPosRightKnee, float errorRotHead, float errorRotHip, float errorRotLeftHand, float errorRotLeftForeArm, float errorRotRightHand, float errorRotRightForeArm, float errorRotLeftFoot, float errorRotRightFoot, float errorRotLeftKnee, float errorRotRightKnee);
	void endEvaluationLogger();
	
	// HMM
	void startHMMLogger(const char* filename, int num);
	void endHMMLogger();
	void saveHMMData(const char* tag, float lastTime, Kore::vec3 pos, Kore::Quaternion rot);
	void analyseHMM(const char* filename, double probability, bool newLine);
	
	bool readData(const int numOfEndEffectors, const char* filename, Kore::vec3* rawPos, Kore::Quaternion* rawRot, EndEffectorIndices indices[], float& scale);
};
