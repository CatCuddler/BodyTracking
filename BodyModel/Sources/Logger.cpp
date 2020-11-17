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

void Logger::saveEvaluationData(const char* filename, const float* iterations, float meanErrorPos, float stdErrorPos, float meanErrorRot, float stdErrorRot, const float* time, const float* timeIteration, float reached, float stucked, const float* errorHead, const float* errorHip, const float* errorLeftHand, const float* errorLeftForeArm, const float* errorRightHand, const float* errorRightForeArm, const float* errorLeftFoot, const float* errorRightFoot, const float* errorLeftKnee, const float* errorRightKnee) {
	
	// Save settings
	char evaluationDataPath[100];
	sprintf(evaluationDataPath, "eval/evaluationData_IK_%i_%s", ikMode, filename);
	
	if (!evaluationDataOutputFile.is_open()) {
		evaluationDataOutputFile.open(evaluationDataPath, std::ios::app);
		
		// Append to the end
		evaluationDataOutputFile << "IKMode;File;Lambda;ErrorMaxPos;ErrorMaxRot;IterationsMax;";
		evaluationDataOutputFile << "MeanIterations;StdIterations;";
		evaluationDataOutputFile << "MeanPosError[mm];StdPosError[mm];";
		evaluationDataOutputFile << "MeanRotError[deg];StdRotError[deg];";
		evaluationDataOutputFile << "MeanTimePerIteration[ms];StdTimePerIteration[ms];";
		evaluationDataOutputFile << "MeanTime[ms];StdTime[ms];";
		evaluationDataOutputFile << "Reached[%];Stucked[%];";
		evaluationDataOutputFile << "MeanHeadPosError;StdHeadPosError;MeanHeadRotError;StdHeadRotError;";
		evaluationDataOutputFile << "MeanHipPosError;StdHipPosError;MeanHipRotError;StdHipRotError;";
		evaluationDataOutputFile << "MeanLeftHandPosError;StdLeftHandPosError;MeanLeftHandRotError;StdLeftHandRotError;";
		evaluationDataOutputFile << "MeanLeftForeArmPosError;StdLeftForeArmPosError;MeanLeftForeArmRotError;StdLeftForeArmRotError;";
		evaluationDataOutputFile << "MeanRightHandPosError;StdRightHandPosError;MeanRightHandRotError;StdRightHandRotError;";
		evaluationDataOutputFile << "MeanRightForeArmPosError;StdRightForeArmPosError;MeanRightForeArmRotError;StdRightForeArmRotError;";
		evaluationDataOutputFile << "MeanLeftFootPosError;StdLeftFootPosError;MeanLeftFootRotError;StdLeftFootRotError;";
		evaluationDataOutputFile << "MeanRightFootPosError;StdRightFootPosError;MeanRightFootRotError;StdRightFootRotError;";
		evaluationDataOutputFile << "MeanLeftKneePosError;StdLeftKneePosError;MeanLeftKneeRotError;StdLeftKneeRotError;";
		evaluationDataOutputFile << "MeanRightKneePosError;StdRightKneePosError;MeanRightKneeRotError;StdRightKneeRotError\n";
	}
	
	// Save settings
	log(Kore::Info, "%s \t IK: %i \t lambda: %f \t errorMaxPos: %f \t errorMaxRot: %f \t maxIterations: %f", filename, ikMode, lambda[ikMode], errorMaxPos[ikMode], errorMaxRot[ikMode], maxIterations[ikMode]);
	evaluationDataOutputFile << ikMode << ";" << filename << ";" << lambda[ikMode] << ";" << errorMaxPos[ikMode] << ";" << errorMaxRot[ikMode] << ";" << maxIterations[ikMode] << ";";

	// Save mean and std for iterations
	evaluationDataOutputFile << iterations[0] << ";" << iterations[1] << ";";

	// Save mean and std for pos and rot error
	evaluationDataOutputFile << meanErrorPos << ";" << stdErrorPos << ";";
	evaluationDataOutputFile << meanErrorRot << ";" << stdErrorRot << ";";
	
	// Save time
	evaluationDataOutputFile << timeIteration[0] << ";" << timeIteration[1] << ";";
	evaluationDataOutputFile << time[0] << ";" << time[1] << ";";

	// Reached & stucked
	evaluationDataOutputFile << reached << ";" << stucked << ";";
	
	// Save results for each individual bone
	evaluationDataOutputFile << errorHead[0] << ";" << errorHead[1] << ";" << errorHead[2] << ";" << errorHead[3] << ";";
	evaluationDataOutputFile << errorHip[0] << ";" << errorHip[1] << ";" << errorHip[2] << ";" << errorHip[3] << ";";
	evaluationDataOutputFile << errorLeftHand[0] << ";" << errorLeftHand[1] << ";" << errorLeftHand[2] << ";" << errorLeftHand[3] << ";";
	evaluationDataOutputFile << errorLeftForeArm[0] << ";" << errorLeftForeArm[1] << ";" << errorLeftForeArm[2] << ";" << errorLeftForeArm[3] << ";";
	evaluationDataOutputFile << errorRightHand[0] << ";" << errorRightHand[1] << ";" << errorRightHand[2] << ";" << errorRightHand[3] << ";";
	evaluationDataOutputFile << errorRightForeArm[0] << ";" << errorRightForeArm[1] << ";" << errorRightForeArm[2] << ";" << errorRightForeArm[3] << ";";
	evaluationDataOutputFile << errorLeftFoot[0] << ";" << errorLeftFoot[1] << ";" << errorLeftFoot[2] << ";" << errorLeftFoot[3] << ";";
	evaluationDataOutputFile << errorRightFoot[0] << ";" << errorRightFoot[1] << ";" << errorRightFoot[2] << ";" << errorRightFoot[3] << ";";
	evaluationDataOutputFile << errorLeftKnee[0] << ";" << errorLeftKnee[1] << ";" << errorLeftKnee[2] << ";" << errorLeftKnee[3] << ";";
	evaluationDataOutputFile << errorRightKnee[0] << ";" << errorRightKnee[1] << ";" << errorRightKnee[2] << ";" << errorRightKnee[3] << "\n";
	
	evaluationDataOutputFile.flush();
}

void Logger::endEvaluationLogger() {
	evaluationDataOutputFile.flush();
	evaluationDataOutputFile.close();
	
	log(Kore::Info, "Stop eval-logging!");
}

bool Logger::readData(const int numOfEndEffectors, const char* filename, Kore::vec3* rawPos, Kore::Quaternion* rawRot, EndEffectorIndices indices[], float& scale) {
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
		
		EndEffectorIndices endEffectorIndex = unknown;
		if(std::strcmp(tag.c_str(), headTag) == 0)			endEffectorIndex = head;
		else if(std::strcmp(tag.c_str(), hipTag) == 0)		endEffectorIndex = hip;
		else if(std::strcmp(tag.c_str(), lHandTag) == 0)	endEffectorIndex = leftHand;
		else if(std::strcmp(tag.c_str(), rHandTag) == 0)	endEffectorIndex = rightHand;
		else if (std::strcmp(tag.c_str(), lForeArm) == 0)	endEffectorIndex = leftForeArm;
		else if (std::strcmp(tag.c_str(), rForeArm) == 0)	endEffectorIndex = rightForeArm;
		else if (std::strcmp(tag.c_str(), lFootTag) == 0)	endEffectorIndex = leftFoot;
		else if (std::strcmp(tag.c_str(), rFootTag) == 0)	endEffectorIndex = rightFoot;
		else if (std::strcmp(tag.c_str(), lKneeTag) == 0)	endEffectorIndex = leftKnee;
		else if (std::strcmp(tag.c_str(), rKneeTag) == 0)	endEffectorIndex = rightKnee;
		
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
