#pragma once

#include <Kore/Math/Vector.h>

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

struct BoneNode;

class MoCap {

private:
	const char* mocapFilepath = "mocap/mocap_29.txt";	//jumping jacks, side twists, bend over, squats
	
	int headerMocap = 3;
	int currTxtLineNumber = 0;
	std::fstream positionMocapData;
	
	std::string readLine(Kore::vec3* rawPos);
	
public:
	MoCap(std::vector<BoneNode*> bones);
	
	void readMocalSet(Kore::vec3* rawPos);
};
