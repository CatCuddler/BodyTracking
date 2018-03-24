#pragma once

#include <Kore/Math/Vector.h>

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

struct BoneNode;

class MoCap {

private:
	
	int headerMocap = 4;
	int currTxtLineNumber = 0;
	std::fstream positionMocapData;
	
	

	
public:
	MoCap(std::vector<BoneNode*> bones);
	
	void setBone(BoneNode* bone, Kore::vec3 desiredPosition);
	
	void readMocalSet(std::string* boneName, Kore::vec3* rawPos);
	void readMocapData(const char* filename, std::string* boneName, Kore::vec3* rawPos);

};
