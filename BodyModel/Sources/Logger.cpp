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
		const char* filename = "positionData";
		std::stringstream filePath;
		
		std::fstream inputFile;
	}
}

void Logger::saveData(Kore::vec3 rawPos, Kore::Quaternion rawRot) {
	time_t t = time(0);   // get time now
	
	std::fstream outputFile;
	
	if (Kore::Logger::filePath.str().empty()) {
		Kore::Logger::filePath << Kore::Logger::filename << "_" << t << ".csv";
	}
	
	outputFile.open(Kore::Logger::filePath.str(), std::ios::app);	// Append to the end
	
	//outputFile << "rawX, rawY, rawZ, targetX, targetY, targetZ\n";
	
	outputFile << rawPos.x() << "," << rawPos.y() << "," << rawPos.z() << "," << rawRot.x << "," << rawRot.y << "," << rawRot.z << "," << rawRot.w << "\n";
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
