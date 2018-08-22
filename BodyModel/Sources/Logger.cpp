#include "pch.h"

#include "Logger.h"

#include <Kore/Log.h>

#include <ctime>

Logger::Logger() : initPositionData(false), initTransRotData(false), initLogData(false), initHmmAnalysisData(false) {
	time_t t = time(0);   // Get time now
	positionDataPath << positionData << "_" << t << ".csv";
	headerTrackingData = "name;timestamp;rawPosX;rawPosY;rawPosZ;rawRotX;rawRotY;rawRotZ;rawRotW\n";
	headerTrackingDataLength = (int)strlen(headerTrackingData);
	
	initTransRotPath << initTransRotFilename << "_" << t << ".csv";
	logDataPath << logDataFilename << "_" << t << ".csv";
	
	curentFileNumber = 0;
	curentLineNumber = 0;
}

Logger::~Logger() {
	positionDataOutputFile.close();
	logDataOutputFile.close();
	hmmAnalysisOutputFile.close();
}

void Logger::saveData(float timestamp, std::string name, Kore::vec3 rawPos, Kore::Quaternion rawRot) {
	if (!initPositionData) {
		positionDataPath.str(std::string());
		positionDataPath << positionData << "_" << curentFileNumber << ".csv";

		curentFileNumber++;
		curentLineNumber = 0;
		
		positionDataOutputFile.open(positionDataPath.str(), std::ios::out);
		positionDataOutputFile << headerTrackingData;

		// placeholder for line number that will be overwritten when the file is closed
		positionDataOutputFile << "N=        \n";

		positionDataOutputFile.flush();
		initPositionData = true;
	}
	
	curentLineNumber++;

	// Save positional and rotation data
	positionDataOutputFile << name << ";" << timestamp << ";" << rawPos.x() << ";" << rawPos.y() << ";" << rawPos.z() << ";" << rawRot.x << ";" << rawRot.y << ";" << rawRot.z << ";" << rawRot.w << "\n";
	positionDataOutputFile.flush();
}

void Logger::saveInitTransAndRot(Kore::vec3 initPos, Kore::Quaternion initRot) {
	if (!initTransRotData) {
		initTransRotDataOutputFile.open(initTransRotPath.str(), std::ios::app);
		initTransRotDataOutputFile << "initPosX;initPosY;initPosZ;initRotX;initRotY;initRotZ;initRotW\n";
		initTransRotDataOutputFile.flush();
		initTransRotData = true;
	}
	
	// Save initial position rotation
	initTransRotDataOutputFile << initPos.x() << ";" << initPos.y() << ";" << initPos.z() << ";" << initRot.x << ";" << initRot.y << ";" << initRot.z << ";" << initRot.w << "\n";
	initTransRotDataOutputFile.flush();
	initTransRotDataOutputFile.close();
}

void Logger::closeFile() {
	initPositionData = false;
	
	positionDataOutputFile.seekp(headerTrackingDataLength);
	
	positionDataOutputFile << "N=" << curentLineNumber;	// store number of lines / datapoints
	positionDataOutputFile.flush();
	positionDataOutputFile.close();
}

void Logger::saveLogData(const char* str, float num) {
	if (!initLogData) {
		logDataOutputFile.open(logDataPath.str(), std::ios::app);
		initLogData = true;
	}
	
	// Save data
	logDataOutputFile << str << ";" << num << "\n";
	logDataOutputFile.flush();
}

void Logger::analyseHMM(const char* hmmName, double probability, bool newLine) {
	if (!initHmmAnalysisData) {
		hmmAnalysisPath << hmmName << "_analysis.txt";
		hmmAnalysisOutputFile.open(hmmAnalysisPath.str(), std::ios::out | std::ios::app);
		initHmmAnalysisData = true;
	}

	if (newLine) hmmAnalysisOutputFile << "\n";
	else hmmAnalysisOutputFile << probability << ";";

	hmmAnalysisOutputFile.flush();
}

bool Logger::readLine(std::string str, Kore::vec3* rawPos, Kore::Quaternion* rawRot) {
	int column = 0;
	if (std::getline(positionDataInputFile, str, '\n')) {
		
		std::stringstream ss;
		ss.str(str);
		std::string item;
		while(std::getline(ss, item, ';')) {
			float num = std::stof(item);
			//log(Kore::Info, "%f", num);
			
			if (column == 0) rawPos->x() = num;
			else if (column == 1) rawPos->y() = num;
			else if (column == 2) rawPos->z() = num;
			else if (column == 3) rawRot->x = num;
			else if (column == 4) rawRot->y = num;
			else if (column == 5) rawRot->z = num;
			else if (column == 6) rawRot->w = num;
			
			++column;
		}
		
		++curentLineNumber;
		return true;
	} else {
		return false;
	}
}

bool Logger::readData(int line, const int numOfEndEffectors, const char* filename, Kore::vec3* rawPos, Kore::Quaternion* rawRot) {
	if (!positionDataInputFile.is_open()) {
		positionDataInputFile.open(filename);
	}
	
	std::string str;
	
	// Get header
	if (line == 0) {
		std::getline(positionDataInputFile, str, '\n');
		++curentLineNumber;
	}
	
	// Read line
	bool success = false;
	for (int i = 0; i < numOfEndEffectors; ++i) {
		rawPos[i] = Kore::vec3(0, 0, 0);
		rawRot[i] = Kore::Quaternion(0, 0, 0, 1);
		success = readLine(str, &rawPos[i], &rawRot[i]);
	}
	return success;
}

void Logger::readInitTransAndRot(const char* filename, Kore::vec3* initPos, Kore::Quaternion* initRot) {
	std::fstream inputFile(filename);
	
	// Get header
	int column = 0;
	std::string str;
	std::getline(inputFile, str, '\n');
	
	// Get initial positional vector and rotation
	std::getline(inputFile, str, '\n');
	std::stringstream ss;
	ss.str(str);
	std::string item;
	while(std::getline(ss, item, ';')) {
		float num = std::stof(item);
			
		//log(Kore::Info, "%i -> %f", column, num);
		
		if (column == 0) initPos->x() = num;
		else if (column == 1) initPos->y() = num;
		else if (column == 2) initPos->z() = num;
		else if (column == 3) initRot->x = num;
		else if (column == 4) initRot->y = num;
		else if (column == 5) initRot->z = num;
		else if (column == 6) initRot->w = num;
		
		++column;
	}
}
