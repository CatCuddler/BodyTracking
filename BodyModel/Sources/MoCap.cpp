#include "pch.h"

#include "MoCap.h"

#include <Kore/Log.h>

MoCap::MoCap() {

}

void MoCap::readMocalSet(std::string* boneName, Kore::vec3* rawPos) {
	std::string str;
	int column = 0;
	if (std::getline(positionMocapData, str, '\n')) {
		
		std::stringstream ss;
		ss.str(str);
		std::string item;
		while(std::getline(ss, item, ' ')) {
			float num = -1;
			if (column > 0) {
				num = std::stof(item);
			}
			//log(Kore::Info, "%f", num);
			
			if (column == 0) *boneName = item;
			else if (column == 1) rawPos->x() = num;
			else if (column == 2) rawPos->y() = num;
			else if (column == 3) rawPos->z() = num;
			
			
			++column;
		}
		
		++currTxtLineNumber;
	}
	
}

void MoCap::readMocapData(const char* filename, std::string* boneName, Kore::vec3* rawPos) {
	if (!positionMocapData.is_open()) {
		positionMocapData.open(filename);
	}
	
	std::string str;
	
	// Get header
	while (headerMocap > 0) {
		std::getline(positionMocapData, str, '\n');
		--headerMocap;
	}
	
	//std::getline(positionMocapData, str, '\n');
	//++currTxtLineNumber;
	
	// TODO
	readMocalSet(boneName, rawPos);
	
	
	//Kore::log(Kore::Info, "bone name %s -> %f %f %f", boneName, rawPos->x(), rawPos->y(), rawPos->z());
	
}
