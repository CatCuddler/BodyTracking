#include "pch.h"

#include "MoCap.h"

#include <Kore/Log.h>

MoCap::MoCap(std::vector<BoneNode*> bones) {

}

std::string MoCap::readLine(Kore::vec3* rawPos) {
	std::string boneName = "";
	
	std::string str;
	int column = 0;
	if (std::getline(positionMocapData, str, '\n')) {
		
		std::stringstream ss;
		ss.str(str);
		std::string item;
		
		double nums[6];
		
		while(std::getline(ss, item, ' ')) {
			double num = -1;
			if (column > 0) {
				num = std::stof(item);
			}
			//log(Kore::Info, "%f", num);
			
			if (column == 0) boneName = item;
			else if (column == 1) nums[0] = num;
			else if (column == 2) nums[1] = num;
			else if (column == 3) nums[2] = num;
			else if (column == 4) nums[3] = num;
			else if (column == 5) nums[4] = num;
			else if (column == 6) nums[5] = num;

			
			++column;
		}
		
		float scale = (1.0/0.45) * 2.54/100.0;
		
		//if (std::strcmp(boneName.c_str(), "root") == 0)				{ rawPos->x() = nums[3]; rawPos->y() = nums[4]; rawPos->z() = nums[5]; }
		if (std::strcmp(boneName.c_str(), "root") == 0)				{ rawPos->x() = nums[0] * scale; rawPos->y() = nums[1] * scale; rawPos->z() = nums[2] * scale; }
		else if (std::strcmp(boneName.c_str(), "lowerback") == 0)	{ rawPos->x() = nums[0]; rawPos->y() = nums[1]; rawPos->z() = nums[2]; }
		else if (std::strcmp(boneName.c_str(), "upperback") == 0)	{ rawPos->x() = nums[0]; rawPos->y() = nums[1]; rawPos->z() = nums[2]; }
		else if (std::strcmp(boneName.c_str(), "thorax") == 0)		{ rawPos->x() = nums[0]; rawPos->y() = nums[1]; rawPos->z() = nums[2]; }
		
		else if (std::strcmp(boneName.c_str(), "rclavicle") == 0)	{ rawPos->x() = 0; rawPos->y() = nums[0]; rawPos->z() = nums[1]; }
		else if (std::strcmp(boneName.c_str(), "rhumerus") == 0)	{ rawPos->x() = nums[0]; rawPos->y() = nums[1]; rawPos->z() = nums[2]; }
		else if (std::strcmp(boneName.c_str(), "rradius") == 0)		{ rawPos->x() = nums[0]; rawPos->y() = 0; rawPos->z() = 0; }
		
		else if (std::strcmp(boneName.c_str(), "lclavicle") == 0)	{ rawPos->x() = 0; rawPos->y() = nums[0]; rawPos->z() = nums[1]; }
		else if (std::strcmp(boneName.c_str(), "lhumerus") == 0)	{ rawPos->x() = nums[0]; rawPos->y() = nums[1]; rawPos->z() = nums[2]; }
		else if (std::strcmp(boneName.c_str(), "lradius") == 0)		{ rawPos->x() = nums[0]; rawPos->y() = 0; rawPos->z() = 0; }
		
		else if (std::strcmp(boneName.c_str(), "rfemur") == 0)		{ rawPos->x() = nums[0]; rawPos->y() = nums[1]; rawPos->z() = nums[2]; }
		else if (std::strcmp(boneName.c_str(), "rtibia") == 0)		{ rawPos->x() = nums[0]; rawPos->y() = 0; rawPos->z() = 0; }
		
		else if (std::strcmp(boneName.c_str(), "lfemur") == 0)		{ rawPos->x() = nums[0]; rawPos->y() = nums[1]; rawPos->z() = nums[2]; }
		else if (std::strcmp(boneName.c_str(), "ltibia") == 0)		{ rawPos->x() = nums[0]; rawPos->y() = 0; rawPos->z() = 0; }

		
		
		++currTxtLineNumber;
	}
	
	return boneName;
	
}

void MoCap::readMocalSet(Kore::vec3* rawPos) {
	if (!positionMocapData.is_open()) {
		positionMocapData.open(mocapFilepath);
	}

	std::string str;
	
	// Get header
	while (headerMocap > 0) {
		std::getline(positionMocapData, str, '\n');
		--headerMocap;
	}
	
	const int bones = 29;
	
	for (int i = 0; i < bones; ++i) {
		
		// Skip set number
		if (i == 0) std::getline(positionMocapData, str, '\n');
		
		// Get bone data
		std::string boneName;
		rawPos[i] = Kore::vec3(0, 0, 0);
		boneName = readLine(&rawPos[i]);
		
		//Kore::log(Kore::Info, "%i %s %f %f %f", i, boneName.c_str(), rawPos[i].x(), rawPos[i].y(), rawPos[i].z());
	}
	
}
