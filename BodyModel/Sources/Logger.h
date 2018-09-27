#pragma once

#include "Avatar.h"

#include <Kore/Math/Quaternion.h>
#include <Kore/IO/FileReader.h>

//#include <iostream>
#include <fstream>
#include <sstream>

class Logger {
	
private:
	// Output file to save raw data
	std::ofstream logDataOutputFile;
	// Output file to save data for hmm
	std::ofstream hmmDataOutputFile;
	std::ofstream hmmAnalysisOutputFile;
	
	const char* evaluationDataFilename = "evaluationData";
	std::fstream evaluationDataOutputFile;
	std::stringstream evaluationDataPath;
	
	const char* evaluationConfigFilename = "evaluationConfig";
	std::fstream evaluationConfigOutputFile;
	std::stringstream evaluationConfigPath;
	
	Kore::FileReader logDataReader;
	
public:
	Logger();
	~Logger();
	
	void startLogger(const char* filename);
	void endLogger();
	void saveData(const char* tag, Kore::vec3 rawPos, Kore::Quaternion rawRot, float scale);
	
	void startEvaluationLogger(const char* filename, int ikMode, float lambda, float errorMaxPos, float errorMaxRot, int maxSteps);
	void saveEvaluationData(Avatar *avatar);
	void endEvaluationLogger();
	
	// HMM
	void startHMMLogger(const char* filename, int num);
	void endHMMLogger(int lineCount);
	void saveHMMData(const char* tag, float lastTime, Kore::vec3 pos);
	void analyseHMM(const char* filename, double probability, bool newLine);
	
	bool readData(const int numOfEndEffectors, const char* filename, Kore::vec3* rawPos, Kore::Quaternion* rawRot, float& scale);
};
