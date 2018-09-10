#pragma once

#include "Avatar.h"

#include <Kore/Graphics4/Graphics.h>
#include <Kore/Math/Quaternion.h>

#include <iostream>
#include <fstream>
#include <sstream>

class Logger {
	
private:
	// Output file to save raw data
	std::fstream logDataOutputFile;
	// Output file to save data for hmm
	std::fstream hmmDataOutputFile;
	
	const char* evaluationDataFilename = "evaluationData";
	std::fstream evaluationDataOutputFile;
	std::stringstream evaluationDataPath;
	
	const char* evaluationConfigFilename = "evaluationConfig";
	std::fstream evaluationConfigOutputFile;
	std::stringstream evaluationConfigPath;
	
	int currLineNumber = 0;
	std::fstream logDataInputFile;
	bool readLine(std::string str, Kore::vec3* rawPos, Kore::Quaternion* rawRot, float& scale, std::string& tag);
	
public:
	Logger();
	~Logger();
	
	void startLogger(const char* fileName);
	void endLogger();
	void saveData(const char* tag, Kore::vec3 rawPos, Kore::Quaternion rawRot, float scale);
	
	void startEvaluationLogger();
	void endEvaluationLogger();
	
	void saveEvaluationData(Avatar *avatar);
	
	// HMM
	void startHMMLogger(const char* fileName, int num);
	void endHMMLogger();
	void saveHMMData(float lastTime, const char* tag, Kore::vec3 rawPos);
	
	bool readData(const int numOfEndEffectors, const char* filename, Kore::vec3* rawPos, Kore::Quaternion* rawRot, float& scale);
};
