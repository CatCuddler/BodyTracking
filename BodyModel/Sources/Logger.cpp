#include "pch.h"
#include "Logger.h"

#include <Kore/Log.h>

#include <iostream>
#include <string>
#include <ctime>

extern int ikMode;
extern float lambda[];
extern float errorMaxPos[];
extern float errorMaxRot[];
extern float maxIterations[];

namespace {
	bool initHmmAnalysisData = false;
}

Logger::Logger() {
}

Logger::~Logger() {
	logDataReader.close();
	logDataWriter.close();
	hmmWriter.close();
	hmmAnalysisWriter.close();
}

void Logger::startLogger(const char* filename) {
	time_t t = time(0);   // Get time now
	
	char logFileName[50];
	sprintf(logFileName, "%s_%li.csv", filename, t);
	
	logDataWriter.open(logFileName, std::ios::app); // Append to the end
	
	// Append header
	logDataWriter << "tag rawPosX rawPosY rawPosZ rawRotX rawRotY rawRotZ rawRotW scale\n";
	logDataWriter.flush();
	
	log(Kore::Info, "Start logging");
}

void Logger::endLogger() {
	logDataWriter.close();
	
	log(Kore::Info, "Stop logging");
}

void Logger::saveData(const char* tag, Kore::vec3 rawPos, Kore::Quaternion rawRot, float scale) {
	// Save position and rotation
	logDataWriter << tag << " " << rawPos.x() << " " << rawPos.y() << " " << rawPos.z() << " " << rawRot.x << " " << rawRot.y << " " << rawRot.z << " " << rawRot.w << " " << scale << "\n";
	logDataWriter.flush();
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
	hmmWriter << tag << " " << lastTime << " "  << pos.x() << " " << pos.y() << " " << pos.z() << " " << rot.x << " " << rot.y << " " << rot.z << " " << rot.y << "\n";
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

void Logger::saveEvaluationData(const char* filename, const float* iterations, const float* errorPos, const float* errorRot, const float* time, const float* timeIteration, bool reached, bool stucked) {
	
	// Save settings
	char evaluationDataPath[100];
	sprintf(evaluationDataPath, "eval/evaluationData_IK_%i_%s", ikMode, filename);
	
	if (!evaluationDataOutputFile.is_open()) {
		evaluationDataOutputFile.open(evaluationDataPath, std::ios::app);
		
		// Append to the end
		evaluationDataOutputFile << "IK Mode;File;Lambda;Error Max Pos;Error Max Rot;Iterations Max;";
		evaluationDataOutputFile << "Iterations;Error Pos;Error Rot;Error;Time [us];Time/Iteration [us];";
		evaluationDataOutputFile << "Iterations (Min);Error Pos (Min);Error Rot (Min);Error (Min);Time [us] (Min);Time/Iteration [us] (Min);";
		evaluationDataOutputFile << "Iterations (Max);Error Pos (Max);Error Rot (Max);Error (Max);Time [us] (Max);Time/Iteration [us] (Max);";
		evaluationDataOutputFile << "Reached [%];Stucked [%]\n";
	}
	
	// Save settings
	log(Kore::Info, "%s \t IK: %i \t lambda: %f \t errorMaxPos: %f \t errorMaxRot: %f \t maxIterations: %f", filename, ikMode, lambda[ikMode], errorMaxPos[ikMode], errorMaxRot[ikMode], maxIterations[ikMode]);
	evaluationDataOutputFile << ikMode << ";" << filename << ";" << lambda[ikMode] << ";" << errorMaxPos[ikMode] << ";" << errorMaxRot[ikMode] << ";" << maxIterations[ikMode] << ";";
	
	// Save results
	for (int i = 0; i < 3; ++i) {
		float error = Kore::sqrt(Kore::sqrt(*(errorPos + i)) + Kore::sqrt(*(errorRot + i)));
		
		evaluationDataOutputFile << *(iterations + i) << ";";
		evaluationDataOutputFile << *(errorPos + i) << ";";
		evaluationDataOutputFile << *(errorRot + i) << ";";
		evaluationDataOutputFile << error << ";";
		evaluationDataOutputFile << *(time + i) << ";";
		evaluationDataOutputFile << *(timeIteration + i) << ";";
	}
	evaluationDataOutputFile << reached << ";" << stucked << "\n";
	
	evaluationDataOutputFile.flush();
}

void Logger::endEvaluationLogger() {
	evaluationDataOutputFile.flush();
	evaluationDataOutputFile.close();
	
	log(Kore::Info, "Stop eval-logging!");
}

bool Logger::readData(const int numOfEndEffectors, const char* filename, Kore::vec3* rawPos, Kore::Quaternion* rawRot, float& scale) {
	std::string tag;
	float posX, posY, posZ;
	float rotX, rotY, rotZ, rotW;
	
	if(!logDataReader.is_open()) {
		
		if (std::ifstream(filename)) {
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
