#pragma once

#include "Avatar.h"

#include <Kore/Graphics4/Graphics.h>
#include <Kore/Math/Quaternion.h>

#include <iostream>
#include <fstream>
#include <sstream>

extern int currentFile, ikMode;
extern float dMaxPos[], dMaxRot[], lambda[], maxSteps[];

class Logger {
	
private:
	const char* positionData = "positionData";
	std::fstream positionDataOutputFile;
	std::stringstream positionDataPath;
	
	const char* evaluationDataFilename = "evaluationData";
	std::fstream evaluationDataOutputFile;
	std::stringstream evaluationDataPath;
	
	const char* evaluationConfigFilename = "evaluationConfig";
	std::fstream evaluationConfigOutputFile;
	std::stringstream evaluationConfigPath;
	
	int currLineNumber = 0;
	std::fstream positionDataInputFile;
	
	bool readLine(std::string str, Kore::vec3* rawPos, Kore::Quaternion* rawRot, float& scale);
	
public:
	Logger();
	~Logger();
	
	void startLogger();
	void endLogger();
	void saveData(Kore::vec3 rawPos, Kore::Quaternion rawRot, float scale);
	
	void startEvaluationLogger();
	void endEvaluationLogger();
	
	void saveEvaluationData(Avatar *avatar);
	
	bool readData(int line, const int numOfEndEffectors, const char* filename, Kore::vec3* rawPos, Kore::Quaternion* rawRot, float& scale);
};
