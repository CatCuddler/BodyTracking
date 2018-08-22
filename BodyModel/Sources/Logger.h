#pragma once

#include <Kore/Graphics4/Graphics.h>
#include <Kore/Math/Quaternion.h>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

class Logger {
	
private:
	const char* positionData = "Yoga_Krieger";
	const char* initTransRotFilename = "initTransAndRot";
//	const char* logDataFilename = "logData";
	const char* logDataFilename = "Yoga_Krieger";

	const char* analysisFile = "Yoga_Krieger_analysis.txt";
	
	const char* logData = "logData";
	
	std::stringstream positionDataPath;
	std::stringstream initTransRotPath;
	std::stringstream logDataPath;
	std::stringstream hmmAnalysisPath;
	
	bool initPositionData;
	std::ofstream positionDataOutputFile;
	const char* headerTrackingData;
	int headerTrackingDataLength;
	
	bool initTransRotData;
	std::ofstream initTransRotDataOutputFile;
	
	bool initHmmAnalysisData;
	std::ofstream hmmAnalysisOutputFile;
	
	bool initLogData;
	std::ofstream logDataOutputFile;
	
	int curentFileNumber = 0;
	int curentLineNumber = 0;
	
	std::ifstream positionDataInputFile;
	
	bool readLine(std::string str, Kore::vec3* rawPos, Kore::Quaternion* rawRot);
	
public:
	Logger();
	~Logger();
	void saveData(float timestamp, std::string name, Kore::vec3 rawPos, Kore::Quaternion rawRot = Kore::Quaternion(0, 0, 0, 1));
	void saveInitTransAndRot(Kore::vec3 initPos, Kore::Quaternion initRot);
	void closeFile();
	
	void saveLogData(const char* str, float num);
	
	void analyseHMM(const char* hmmName, double probability, bool newLine);

	bool readData(int line, const int numOfEndEffectors, const char* filename, Kore::vec3* rawPos, Kore::Quaternion* rawRot);
	void readInitTransAndRot(const char* filename, Kore::vec3* initPos, Kore::Quaternion* initRot);
};
