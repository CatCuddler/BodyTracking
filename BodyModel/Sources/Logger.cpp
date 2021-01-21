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
	hmmWriter.close();
	hmmAnalysisWriter.close();
	evaluationDataOutputFile.close();
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

void Logger::startHMMLogger(const char* filename, int num) {
	char logFileName[50];
	sprintf(logFileName, "%s_%i.csv", filename, num);
	
	hmmWriter.open(logFileName, std::ios::out);
	
	// Append header
	char hmmHeader[] = "tag time posX posY posZ rotX rotY rotZ rotW\n";
	hmmWriter << hmmHeader;
	hmmWriter.flush();
	
	log(Kore::Info, "Start logging data for HMM");
}

void Logger::endHMMLogger() {
	hmmWriter.flush();
	hmmWriter.close();
	
	log(Kore::Info, "Stop logging data for HMM");
}

void Logger::saveHMMData(const char* tag, float lastTime, Kore::vec3 pos, Kore::Quaternion rot) {
	// Save position
	hmmWriter << tag << " " << lastTime << " "  << pos.x() << " " << pos.y() << " " << pos.z() << " " << rot.x << " " << rot.y << " " << rot.z << " " << rot.w << "\n";
	hmmWriter.flush();
}

void Logger::analyseHMM(const char* hmmName, double probability, bool newLine) {
	if (!initHmmAnalysisData) {
		char hmmAnalysisPath[100];
		strcat(hmmAnalysisPath, hmmName);
		strcat(hmmAnalysisPath, "_analysis.txt");
		hmmAnalysisWriter.open(hmmAnalysisPath, std::ios::out | std::ios::app);
		initHmmAnalysisData = true;
	}
	
	if (newLine) hmmAnalysisWriter << "\n";
	else hmmAnalysisWriter << probability << " ";
	
	hmmAnalysisWriter.flush();
}

void Logger::startEvaluationLogger(const char* filename) {
	time_t t = time(0);   // Get time now
	
	char evaluationDataPath[100];
	sprintf(evaluationDataPath, "eval/%s_%li.csv", filename, t);
	
	evaluationDataOutputFile.open(evaluationDataPath, std::ios::app);	// Append to the end
	
	// Append header
	evaluationDataOutputFile << "Time;NodeID;Pose;Trials;Head;Hip;LeftArm;RightArm;LeftLeg;RightLeg\n";
	evaluationDataOutputFile.flush();
	
	log(Kore::Info, "Start eval-logging!");
}

void Logger::endEvaluationLogger() {
	evaluationDataOutputFile.flush();
	evaluationDataOutputFile.close();
	
	log(Kore::Info, "Stop eval-logging!");
}

void Logger::saveEvaluationData(float lastTime, int nodeID, int pose, int trials, bool head, bool hip, bool left_arm, bool right_arm, bool left_leg, bool right_leg) {
	evaluationDataOutputFile << lastTime << ";" << nodeID << ";" << pose << ";" << trials << ";" << head << ";" << hip << ";" << left_arm << ";" << right_arm << ";" << left_leg << ";" << right_leg << "\n";
	evaluationDataOutputFile.flush();
}

bool Logger::readData(const int numOfEndEffectors, const char* filename, Kore::vec3* rawPos, Kore::Quaternion* rawRot, EndEffectorIndices indices[], float& scale) {
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
		
		EndEffectorIndices endEffectorIndex = unknown;
		if(std::strcmp(tag.c_str(), headTag) == 0)			endEffectorIndex = head;
		else if(std::strcmp(tag.c_str(), hipTag) == 0)		endEffectorIndex = hip;
		else if(std::strcmp(tag.c_str(), lHandTag) == 0)	endEffectorIndex = leftHand;
		else if(std::strcmp(tag.c_str(), rHandTag) == 0)	endEffectorIndex = rightHand;
		else if (std::strcmp(tag.c_str(), lForeArm) == 0)	endEffectorIndex = leftForeArm;
		else if (std::strcmp(tag.c_str(), rForeArm) == 0)	endEffectorIndex = rightForeArm;
		else if (std::strcmp(tag.c_str(), lFootTag) == 0)	endEffectorIndex = leftFoot;
		else if (std::strcmp(tag.c_str(), rFootTag) == 0)	endEffectorIndex = rightFoot;
		
		rawPos[i] = Kore::vec3(posX, posY, posZ);
		rawRot[i] = Kore::Quaternion(rotX, rotY, rotZ, rotW);
		indices[i] = endEffectorIndex;
		
		if (logDataReader.eof()) {
			logDataReader.close();
			return false;
		}
	}
	
	return true;
}
