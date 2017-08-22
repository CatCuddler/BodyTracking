#include "pch.h"

#include "Logger.h"

#include <Kore/Log.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include <ctime>
#include <vector>

namespace Kore {
	namespace Logger {
		const char* positionData = "positionData";
		const char* initTransRotFilename = "initTransAndRot";
		std::stringstream positionDataPath;
		std::stringstream initTransRotPath;
		
		std::fstream inputFile;
	}
}

void Logger::saveData(Kore::vec3 rawPos, Kore::Quaternion rawRot) {
	if (Kore::Logger::positionDataPath.str().empty()) {
		time_t t = time(0);   // Get time now
		Kore::Logger::positionDataPath << Kore::Logger::positionData << "_" << t << ".csv";
	}
	
	std::fstream outputFile;
	outputFile.open(Kore::Logger::positionDataPath.str(), std::ios::app);	// Append to the end
	
	// Save positional and rotation data
	//outputFile << "rawX, rawY, rawZ, targetX, targetY, targetZ\n";
	outputFile << rawPos.x() << "," << rawPos.y() << "," << rawPos.z() << "," << rawRot.x << "," << rawRot.y << "," << rawRot.z << "," << rawRot.w << "\n";
	outputFile.close();
}

void Logger::saveInitTransAndRot(Kore::mat4 initTrans, Kore::Quaternion initRot) {
	time_t t = time(0);   // Get time now
	Kore::Logger::initTransRotPath << Kore::Logger::initTransRotFilename << "_" << t << ".csv";
	
	std::fstream outputFile;
	
	std::stringstream row;
	
	// Save initial transformation matrix
	outputFile.open(Kore::Logger::initTransRotPath.str(), std::ios::app);
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			row << initTrans[i][j];
			if (j < 3) row << ",";
		}
		
		row << "\n";
	}
	
	outputFile << row.rdbuf();
	
	// Save initial rotation
	outputFile << initRot.x << "\n";
	outputFile << initRot.y << "\n";
	outputFile << initRot.z << "\n";
	outputFile << initRot.w << "\n";
	
	outputFile.close();

}

bool Logger::readData(int line, const char* filename, Kore::vec3 *rawPos, Kore::Quaternion *rawRot) {
	if (!Kore::Logger::inputFile.is_open()) {
		Kore::Logger::inputFile.open(filename);
	}
	
	int column = 0;
	
	std::string str;
	if (std::getline(Kore::Logger::inputFile, str, '\n')) {
		
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
