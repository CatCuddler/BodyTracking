#pragma once

#include <Kore/Graphics4/Graphics.h>
#include <Kore/Math/Quaternion.h>

#include <iostream>
#include <fstream>
#include <sstream>

class Logger {
	
private:
	const char* positionData = "positionData";
	const char* initTransRotFilename = "initTransAndRot";
	const char* logDataFilename = "logData";
	
	const char* logData = "logData";
	
	std::stringstream positionDataPath;
	std::stringstream initTransRotPath;
	std::stringstream logDataPath;
	
	bool initPositionData;
	std::fstream positionDataOutputFile;
	
	bool initTransRotData;
	std::fstream initTransRotDataOutputFile;
	
	bool initLogData;
	std::fstream logDataOutputFile;
	
	int currLineNumber = 0;
	std::fstream positionDataInputFile;
	
	bool readLine(std::string str, Kore::vec3* rawPos, Kore::Quaternion* rawRot);
	
public:
	Logger();
	~Logger();
	void saveData(Kore::vec3 rawPos, Kore::Quaternion rawRot);
	void saveInitTransAndRot(Kore::vec3 initPos, Kore::Quaternion initRot);
	
	void saveLogData(const char* str, float num);
	
	bool readData(int line, const int numOfEndEffectors, const char* filename, Kore::vec3* rawPos, Kore::Quaternion* rawRot);
	void readInitTransAndRot(const char* filename, Kore::vec3* initPos, Kore::Quaternion* initRot);
};
