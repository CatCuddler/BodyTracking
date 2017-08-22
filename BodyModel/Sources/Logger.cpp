#include "pch.h"

#include "Logger.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include <ctime>

namespace Kore {
	namespace Logger {
		const char* filename = "positionData";
		std::stringstream filePath;
	}
}

void Logger::saveData(Kore::vec3 rawPos, Kore::Quaternion rawRot) {
	time_t t = time(0);   // get time now
	
	std::fstream outputFile;
	
	if (Kore::Logger::filePath.str().empty()) {
		Kore::Logger::filePath << "Deployment/" << Kore::Logger::filename << "_" << t << ".csv";
	}
	
	outputFile.open(Kore::Logger::filePath.str(), std::ios::app);	// Append to the end
	
	//outputFile << "rawX, rawY, rawZ, targetX, targetY, targetZ\n";
	
	outputFile << rawPos.x() << "," << rawPos.y() << "," << rawPos.z() << "," << rawRot.x << "," << rawRot.y << "," << rawRot.z << "," << rawRot.w << "\n";
	outputFile.close();
}

void readData(int line, Kore::vec3 &rawPos, Kore::Quaternion &rawRot) {
	
}
