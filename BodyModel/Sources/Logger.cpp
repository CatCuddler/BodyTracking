#include "pch.h"

#include "Logger.h"

#include <iostream>
#include <fstream>
#include <sstream>

void Logger::savePositionData(int maxIteration, Kore::vec3 rawPosArray, Kore::vec3 targetPosArray) {
	const char* filename = "positionData";
	std::fstream outputFile;
	std::stringstream filePath;
	filePath << /*"Deployment/results/" <<*/ filename << "_" << maxIteration << ".csv";
	outputFile.open(filePath.str(), std::ios::app);	// Append to the end
	
	//outputFile << "rawX, rawY, rawZ, targetX, targetY, targetZ\n";
	
	outputFile << rawPosArray.x() << "," << rawPosArray.y() << "," << rawPosArray.z() << "," << targetPosArray.x() << "," << targetPosArray.y() << "," << targetPosArray.z() << "\n";
	outputFile.close();

}
