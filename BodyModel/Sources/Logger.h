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
	
	float prevScale;
	float scale;
	
public:
	~Logger();
	
	void startLogger();
	void endLogger();
	
	void startEvaluationLogger();
	void endEvaluationLogger();
	
	void saveData(Kore::vec3 rawPos, Kore::Quaternion rawRot, float scale);
	void saveEvaluationData(Avatar *avatar);
	
	bool readLine(std::string str, Kore::vec3* rawPos, Kore::Quaternion* rawRot);
	bool readData(int line, const int numOfEndEffectors, const char* filename, Kore::vec3* rawPos, Kore::Quaternion* rawRot);
	
	float getScale() {
		return scale;
	}
};
