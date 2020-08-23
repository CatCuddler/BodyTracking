#include "pch.h"
#include "Logger.h"

#include <Kore/Log.h>

#include <iostream>
#include <string>
#include <ctime>

namespace {
	bool initHmmAnalysisData = false;
}

Logger::Logger() {
}

Logger::~Logger() {
	logDataReader.close();
	logdataWriter.close();
	motionRecognitionWriter.close();
}

void Logger::startLogger(const char* filename) {
	time_t t = time(0);   // Get time now
	
	char logFileName[50];
	sprintf(logFileName, "%s_%li.csv", filename, t);
	
	logdataWriter.open(logFileName, std::ios::app); // Append to the end
	
	// Append header
	logdataWriter << "tag rawPosX rawPosY rawPosZ rawRotX rawRotY rawRotZ rawRotW scale\n";
	logdataWriter.flush();
	
	log(Kore::Info, "Start logging");
}

void Logger::endLogger() {
	logdataWriter.close();
	
	log(Kore::Info, "Stop logging");
}

void Logger::saveData(const char* tag, Kore::vec3 rawPos, Kore::Quaternion rawRot, float scale) {
	// Save position and rotation
	logdataWriter << tag << " " << rawPos.x() << " " << rawPos.y() << " " << rawPos.z() << " " << rawRot.x << " " << rawRot.y << " " << rawRot.z << " " << rawRot.w << " " << scale << "\n";
	logdataWriter.flush();
}

void Logger::startMotionRecognitionLogger(const char* filename) {
	time_t t = time(0);   // Get current time

	// create a new file with the given name
	char logFileName[100];
	sprintf(logFileName, "%s_%li.csv", filename, t);
	motionRecognitionWriter.open(logFileName, std::ios::app); // Append to the end

	// Writer the header for the sensor reading table
	motionRecognitionWriter  
		<< "tag;subject;activity;"
		
		<< "rawPosX;rawPosY;rawPosZ;"
		<< "desPosX;desPosY;desPosZ;"
		<< "finalPosX;finalPosY;finalPosZ;"

		<< "rawRotX;rawRotY;rawRotZ;rawRotW;"
		<< "desRotX;desRotY;desRotZ;desRotW;"
		<< "finalRotX;finalRotY;finalRotZ;finalRotW;"

		<< "rawAngVelX;rawAngVelY;rawAngVelZ;"
		<< "desAngVelX;desAngVelY;desAngVelZ;desAngVelW;"
		
		<< "rawLinVelX;rawLinVelY;rawLinVelZ;"
		<< "desLinVelX;desLinVelY;desLinVelZ;"

		<< "scale;time\n";
	motionRecognitionWriter.flush();
}

void Logger::endMotionRecognitionLogger() {
	motionRecognitionWriter.flush();
	motionRecognitionWriter.close();
}

void Logger::saveMotionRecognitionData(
	const char* tag, const char* subject, const char* activity, 
	Kore::vec3 rawPos, Kore::vec3 desPos, Kore::vec3 finalPos,
	Kore::Quaternion rawRot, Kore::Quaternion desRot, Kore::Quaternion finalRot,
	Kore::vec3 rawAngVel, Kore::Quaternion desAngVel,
	Kore::vec3 rawLinVel, Kore::vec3 desLinVel,
	float scale, double time) {

	// Save position, rotation, angular and linear velocity
	motionRecognitionWriter 
		<< tag << ";" << subject << ";" << activity << ";"
		
		<< rawPos.x() << ";" << rawPos.y() << ";" << rawPos.z() << ";"
		<< desPos.x() << ";" << desPos.y() << ";" << desPos.z() << ";"
		<< finalPos.x() << ";" << finalPos.y() << ";" << finalPos.z() << ";"
		
		<< rawRot.x << ";" << rawRot.y << ";" << rawRot.z << ";" << rawRot.w << ";"
		<< desRot.x << ";" << desRot.y << ";" << desRot.z << ";" << desRot.w << ";"
		<< finalRot.x << ";" << finalRot.y << ";" << finalRot.z << ";" << finalRot.w << ";"
		
		<< rawAngVel.x() << ";" << rawAngVel.y() << ";" << rawAngVel.z() << ";"
		<< desAngVel.x << ";" << desAngVel.y << ";" << desAngVel.z << ";" << desAngVel.w << ";"
		
		<< rawLinVel.x() << ";" << rawLinVel.y() << ";" << rawLinVel.z() << ";"
		<< desLinVel.x() << ";" << desLinVel.y() << ";" << desLinVel.z() << ";"

		<< scale << ";" << time << "\n";
	motionRecognitionWriter.flush();
}

bool Logger::readData(const int numOfEndEffectors, const char* filename, Kore::vec3* rawPos, Kore::Quaternion* rawRot, float& scale) {
	string tag;
	float posX, posY, posZ;
	float rotX, rotY, rotZ, rotW;
	
	if(!logDataReader.is_open()) {
		
		if (ifstream(filename)) {
			logDataReader.open(filename);
			log(Kore::Info, "Read data from %s", filename);
			
			// Skip header
			logDataReader >> tag >> tag >> tag >> tag >> tag >> tag >> tag >> tag >> tag;
		} else {
			log(Kore::Info, "Could not find file %s", filename);
		}
	}
	
	// Read lines
	for (int i = 0; i < numOfEndEffectors; ++i) {
		logDataReader >> tag >> posX >> posY >> posZ >> rotX >> rotY >> rotZ >> rotW >> scale;
		
		rawPos[i] = Kore::vec3(posX, posY, posZ);
		rawRot[i] = Kore::Quaternion(rotX, rotY, rotZ, rotW);
		
		if (logDataReader.eof()) {
			logDataReader.close();
			return false;
		}
	}
	
	return true;
}
