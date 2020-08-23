#pragma once

#include "Avatar.h"

#include <Kore/Math/Quaternion.h>

#include <fstream>

class Logger {
	
private:
	// Input and output file to for raw data
	std::fstream logDataReader;
	std::ofstream logdataWriter;

	// Output file to save data for MotionRecognition
	std::ofstream motionRecognitionWriter;
	
public:
	Logger();
	~Logger();
	
	void startLogger(const char* filename);
	void endLogger();
	void saveData(const char* tag, Kore::vec3 rawPos, Kore::Quaternion rawRot, float scale);

	// Machine Learning Motion Recognition:
	// Create a new sensor reading table
	void startMotionRecognitionLogger(const char* filename);
	// Stop writing to the previously created sensor reading table
	void endMotionRecognitionLogger();
	// Write sensor data to the previously created sensor reading table
	void saveMotionRecognitionData(
		const char* tag, const char* subject, const char* activity,
		Kore::vec3 rawPos, Kore::vec3 desPos, Kore::vec3 finalPos,
		Kore::Quaternion rawRot, Kore::Quaternion desRot, Kore::Quaternion finalRot,
		Kore::vec3 rawAngVel, Kore::Quaternion desAngVel,
		Kore::vec3 rawLinVel, Kore::vec3 desLinVel,
		float scale, double time);
	
	bool readData(const int numOfEndEffectors, const char* filename, Kore::vec3* rawPos, Kore::Quaternion* rawRot, float& scale);
};
