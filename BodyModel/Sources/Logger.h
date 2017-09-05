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
	
	std::stringstream positionDataPath;
	std::stringstream initTransRotPath;
	
	bool initPositionData;
	std::fstream positionDataOutputFile;
	
	bool initTransRotData;
	std::fstream initTransRotDataOutputFile;
	
	int currLineNumber = 0;
	std::fstream positionDataInputFile;
	
public:
	Logger();
	~Logger();
	void saveData(Kore::vec3 rawPos, Kore::Quaternion rawRot);
	void saveInitTransAndRot(Kore::vec3 initPos, Kore::Quaternion initRot);
	bool readData(int line, const char* filename, Kore::vec3 *rawPos, Kore::Quaternion *rawRot);
	void readInitTransAndRot(const char* filename, Kore::vec3 *initPos, Kore::Quaternion *initRot);
};
