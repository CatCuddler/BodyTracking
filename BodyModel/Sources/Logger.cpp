#include "pch.h"

#include "Logger.h"

#include <Kore/Log.h>

#include <ctime>

Logger::Logger() : initPositionData(false), initTransRotData(false) {
	time_t t = time(0);   // Get time now
	positionDataPath << positionData << "_" << t << ".csv";
	initTransRotPath << initTransRotFilename << "_" << t << ".csv";
}

Logger::~Logger() {
	positionDataOutputFile.close();
}

void Logger::saveData(Kore::vec3 rawPos, Kore::Quaternion rawRot) {
	if (!initPositionData) {
		positionDataOutputFile.open(positionDataPath.str(), std::ios::app); // Append to the end
		positionDataOutputFile << "rawPosX, rawPosY, rawPosZ, rawRotX, rawRotY, rawRotZ, rawRotW\n";
		initPositionData = true;
	}
	
	// Save positional and rotation data
	positionDataOutputFile << rawPos.x() << "," << rawPos.y() << "," << rawPos.z() << "," << rawRot.x << "," << rawRot.y << "," << rawRot.z << "," << rawRot.w << "\n";
}

void Logger::saveInitTransAndRot(Kore::mat4 initTrans, Kore::Quaternion initRot) {
	if (!initTransRotData) {
		initTransRotDataOutputFile.open(initTransRotPath.str(), std::ios::app);
		initTransRotData = true;
	}
	
	std::stringstream row;
	// Save initial transformation matrix
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			row << initTrans[i][j];
			if (j < 3) row << ",";
		}
		row << "\n";
	}
	
	initTransRotDataOutputFile << row.rdbuf();
	
	// Save initial rotation
	initTransRotDataOutputFile << initRot.x << "\n";
	initTransRotDataOutputFile << initRot.y << "\n";
	initTransRotDataOutputFile << initRot.z << "\n";
	initTransRotDataOutputFile << initRot.w << "\n";
	
	initTransRotDataOutputFile.close();
}

bool Logger::readData(int line, const char* filename, Kore::vec3 *rawPos, Kore::Quaternion *rawRot) {
	if (!positionDataInputFile.is_open()) {
		positionDataInputFile.open(filename);
	}
	
	std::string str;
	
	// Get header
	if (line == 0) {
		std::getline(positionDataInputFile, str, '\n');
	}
	
	// Skip lines
	while(line > currLineNumber - 1) {
		std::getline(positionDataInputFile, str, '\n');
		++currLineNumber;
	}
	
	// Read line
	int column = 0;
	if (std::getline(positionDataInputFile, str, '\n')) {
		
		std::stringstream ss;
		ss.str(str);
		std::string item;
		while(std::getline(ss, item, ',')) {
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

void Logger::readInitTransAndRot(const char* filename, Kore::mat4 *initTrans, Kore::Quaternion *initRot) {
	std::fstream inputFile(filename);
	
	// Get initial transformation matrix
	int column = 0;
	std::string str;
	for (int row = 0; row < 4; ++row) {
		std::getline(inputFile, str, '\n');
		
		std::stringstream ss;
		ss.str(str);
		std::string item;
		while(std::getline(ss, item, ',')) {
			float num = std::stof(item);
			
			//log(Kore::Info, "%i %i -> %f", row, column, num);
			
			initTrans->Set(row, column, num);
			
			++column;
		}
		column = 0;
	}
	
	// Get initial rotation
	column = 0;
	while (std::getline(inputFile, str, '\n')) {
		float num = std::stof(str);
		
		//log(Kore::Info, "%i -> %f", column, num);
		
		if (column == 0) initRot->x = num;
		else if (column == 1) initRot->y = num;
		else if (column == 2) initRot->z = num;
		else if (column == 3) initRot->w = num;
		
		++column;
	}
}
