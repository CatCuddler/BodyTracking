#pragma once

#include <Kore/Math/Vector.h>

#include <iostream>
#include <fstream>
#include <sstream>

class MoCap {

private:
	
	int headerMocap = 4;
	int currTxtLineNumber = 0;
	std::fstream positionMocapData;
	
	

	
public:
	MoCap();
	
	void readMocalSet(std::string* boneName, Kore::vec3* rawPos);
	void readMocapData(const char* filename, std::string* boneName, Kore::vec3* rawPos);

};
