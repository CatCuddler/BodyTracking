#pragma once

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
	std::fstream evaluationConfigOutputFile;
	
public:
	Logger();
	~Logger();
	
	void startLogger(const char* filename);
	void endLogger();
	void saveData(const char* tag, Kore::vec3 rawPos, Kore::Quaternion rawRot, float scale);
	
	void startEvaluationLogger(const char* filename, int ikMode, float lambda, float errorMaxPos, float errorMaxRot, int maxIterations);
	void saveEvaluationData(const float* iterations, const float* errorPos, const float* errorRot, const float* time, const float* timeIteration, bool reached, bool stucked);
	void endEvaluationLogger();
	
	// HMM
	void startHMMLogger(const char* filename, int num);
	void endHMMLogger();
	void saveHMMData(const char* tag, float lastTime, Kore::vec3 pos, Kore::Quaternion rot);
	void analyseHMM(const char* filename, double probability, bool newLine);
	
	bool readData(const int numOfEndEffectors, const char* filename, Kore::vec3* rawPos, Kore::Quaternion* rawRot, float& scale);
};
