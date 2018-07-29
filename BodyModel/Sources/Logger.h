#pragma once

#include "Avatar.h"

#include <Kore/Graphics4/Graphics.h>
#include <Kore/Math/Quaternion.h>

#include <iostream>
#include <fstream>
#include <sstream>

class Logger {
	
private:
	const char* positionData = "positionData";
	bool initPositionData;
	std::fstream positionDataOutputFile;
	std::stringstream positionDataPath;
	
	const char* initTransRotFilename = "initTransAndRot";
	bool initTransRotData;
	std::fstream initTransRotDataOutputFile;
	std::stringstream initTransRotPath;
	
	const char* logDataFilename = "logData";
	bool initLogData;
	std::fstream logDataOutputFile;
	std::stringstream logDataPath;
	
	const char* evaluationDataFilename = "evaluationData";
	std::fstream evaluationDataOutputFile;
	std::stringstream evaluationDataPath;
	
	const char* evaluationConfigFilename = "evaluationConfig";
	std::fstream evaluationConfigOutputFile;
	std::stringstream evaluationConfigPath;
	
	int currLineNumber = 0;
	std::fstream positionDataInputFile;
	
public:
	Logger();
	~Logger();
	
	void saveData(Kore::vec3 rawPos, Kore::Quaternion rawRot);
	void saveInitTransAndRot(Kore::vec3 initPos, Kore::Quaternion initRot);
	void saveLogData(const char* str, float num);
	void saveEvaluationData(Avatar *avatar);
	
	bool readLine(std::string str, Kore::vec3* rawPos, Kore::Quaternion* rawRot);
	bool readData(int line, const int numOfEndEffectors, const char* filename, Kore::vec3* rawPos, Kore::Quaternion* rawRot);
	void readInitTransAndRot(const char* filename, Kore::vec3* initPos, Kore::Quaternion* initRot);
};
