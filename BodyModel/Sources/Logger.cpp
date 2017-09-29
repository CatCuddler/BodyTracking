#include "pch.h"

#include "Logger.h"

#include <Kore/Log.h>

#include <ctime>

Logger::Logger() : initPositionData(false), initTransRotData(false), initLogData(false) {
	time_t t = time(0);   // Get time now
	positionDataPath << positionData << "_" << t << ".csv";
	initTransRotPath << initTransRotFilename << "_" << t << ".csv";
	logDataPath << logDataFilename << "_" << t << ".csv";
}

Logger::~Logger() {
	positionDataOutputFile.close();
	logDataOutputFile.close();
}

void Logger::saveData(Kore::vec3 rawPos, Kore::Quaternion rawRot) {
	if (!initPositionData) {
		positionDataOutputFile.open(positionDataPath.str(), std::ios::app); // Append to the end
		positionDataOutputFile << "rawPosX;rawPosY;rawPosZ;rawRotX;rawRotY;rawRotZ;rawRotW\n";
		positionDataOutputFile.flush();
		initPositionData = true;
	}
	
	// Save positional and rotation data
	positionDataOutputFile << rawPos.x() << ";" << rawPos.y() << ";" << rawPos.z() << ";" << rawRot.x << ";" << rawRot.y << ";" << rawRot.z << ";" << rawRot.w << "\n";
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

void Logger::saveLogData(const char* str, float num) {
	if (!initLogData) {
		logDataOutputFile.open(logDataPath.str(), std::ios::app);
		initLogData = true;
	}
	
	// Save data
	logDataOutputFile << str << ";" << num << "\n";
	logDataOutputFile.flush();
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
		
		++currLineNumber;
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
		++currLineNumber;
	}
	
	// Skip lines
	while(line > currLineNumber - 1) {
		std::getline(positionDataInputFile, str, '\n');
		++currLineNumber;
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
