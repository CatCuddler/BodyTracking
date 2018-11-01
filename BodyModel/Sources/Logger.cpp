#include "pch.h"
#include "Logger.h"
#include "EndEffector.h"

#include <Kore/Log.h>
#include <ctime>

namespace {
	bool initHmmAnalysisData = false;
	int hmmHeaderLength;
}

Logger::Logger() {
}

Logger::~Logger() {
	logDataOutputFile.close();
	logDataInputFile.close();
	hmmDataOutputFile.close();
	hmmAnalysisOutputFile.close();
}

void Logger::startLogger(const char* filename) {
	time_t t = time(0);   // Get time now
	
	std::stringstream logFileName;
	logFileName << filename << "_" << t << ".csv";
	
	logDataOutputFile.open(logFileName.str(), std::ios::app); // Append to the end
	logDataOutputFile << "tag;rawPosX;rawPosY;rawPosZ;rawRotX;rawRotY;rawRotZ;rawRotW;scale\n";
	logDataOutputFile.flush();
	
	log(Kore::Info, "Start logging");
}

void Logger::endLogger() {
	logDataOutputFile.close();
	
	log(Kore::Info, "Stop logging");
}

void Logger::saveData(const char* tag, Kore::vec3 rawPos, Kore::Quaternion rawRot, float scale) {
	// Save positional and rotation data
	logDataOutputFile << tag << ";" << rawPos.x() << ";" << rawPos.y() << ";" << rawPos.z() << ";" << rawRot.x << ";" << rawRot.y << ";" << rawRot.z << ";" << rawRot.w << ";" << scale << "\n";
	logDataOutputFile.flush();
}

void Logger::startHMMLogger(const char* filename, int num) {
	std::stringstream logFileName;
	logFileName << filename << "_" << num << ".txt";
	
	hmmDataOutputFile.open(logFileName.str(), std::ios::out);
	const char* hmmHeader = "tag;time;posX;posY;posZ\n";
	hmmDataOutputFile << hmmHeader;
	
	// Placeholder for line number that will be overwritten when the file is closed
	hmmDataOutputFile << "N=     \n";
	hmmHeaderLength = (int)strlen(hmmHeader);
	
	hmmDataOutputFile.flush();
	
	log(Kore::Info, "Start logging data for HMM");
}

void Logger::endHMMLogger(int lineCount) {
	hmmDataOutputFile.seekp(hmmHeaderLength);
	// Store number of lines / datapoints
	hmmDataOutputFile <<endl << "N=" << lineCount;
	hmmDataOutputFile.flush();
	hmmDataOutputFile.close();
	
	log(Kore::Info, "Stop logging data for HMM");
}

void Logger::saveHMMData(const char* tag, float lastTime, Kore::vec3 pos) {
	// Save positional and rotation data
	hmmDataOutputFile << tag << ";" << lastTime << ";"  << pos.x() << ";" << pos.y() << ";" << pos.z() << "\n";
	hmmDataOutputFile.flush();
}

void Logger::analyseHMM(const char* hmmName, double probability, bool newLine) {
	if (!initHmmAnalysisData) {
		std::stringstream hmmAnalysisPath;
		hmmAnalysisPath << hmmName << "_analysis.txt";
		hmmAnalysisOutputFile.open(hmmAnalysisPath.str(), std::ios::out | std::ios::app);
		initHmmAnalysisData = true;
	}
	
	if (newLine) hmmAnalysisOutputFile << "\n";
	else hmmAnalysisOutputFile << probability << ";";
	
	hmmAnalysisOutputFile.flush();
}

void Logger::startEvaluationLogger() {
	time_t t = time(0);   // Get time now
	
	evaluationDataPath.str(std::string());
	evaluationConfigPath.str(std::string());
	evaluationDataPath << evaluationDataFilename << "_" << t << ".csv";
	evaluationConfigPath << evaluationConfigFilename << "_" << t << ".csv";
	
	evaluationConfigOutputFile.open(evaluationConfigPath.str(), std::ios::app);
	evaluationConfigOutputFile << "IK Mode;with Orientation;File;lambda;Error Pos Max;Error Rot Max;dMax Pos;dMax Rot;Steps Max\n";
	evaluationConfigOutputFile << ikMode << ";" << withOrientation << ";" << files[currentFile] << ";" << lambda[ikMode] << ";" << errorMaxPos << ";" << errorMaxRot << ";" << dMaxPos[ikMode] << ";" << dMaxRot[ikMode] << ";" << maxSteps[ikMode] << "\n";
	evaluationConfigOutputFile.flush();
	evaluationConfigOutputFile.close();
	
	evaluationDataOutputFile.open(evaluationDataPath.str(), std::ios::app);
	evaluationDataOutputFile << "Iterations;Error Pos;Error Rot;Error;Time [us];Time/Iteration [us];";
	evaluationDataOutputFile << "Iterations Min;Error Pos Min;Error Rot Min;Error Min;Time [us] Min;Time/Iteration [us] Min;";
	evaluationDataOutputFile << "Iterations Max;Error Pos Max;Error Rot Max;Error Max;Time [us] Max;Time/Iteration [us] Max;";
	evaluationDataOutputFile << "Reached [%];Stucked [%]\n";
	evaluationDataOutputFile.flush();
	
	log(Kore::Info, "Start eval-logging!");
}

void Logger::endEvaluationLogger() {
	evaluationDataOutputFile.close();
	
	log(Kore::Info, "Stop eval-logging!");
}

void Logger::saveEvaluationData(Avatar *avatar) {
	float* iterations = avatar->getIterations();
	float* errorPos = avatar->getErrorPos();
	float* errorRot = avatar->getErrorRot();
	float* time = avatar->getTime();
	float* timeIteration = avatar->getTimeIteration();
	
	// Save datas
	for (int i = 0; i < 3; ++i) {
		float error = sqrtf(Square(*(errorPos + i)) + Square(*(errorRot + i)));
		
		evaluationDataOutputFile << *(iterations + i) << ";";
		evaluationDataOutputFile << *(errorPos + i) << ";";
		evaluationDataOutputFile << *(errorRot + i) << ";";
		evaluationDataOutputFile << error << ";";
		evaluationDataOutputFile << *(time + i) << ";";
		evaluationDataOutputFile << *(timeIteration + i) << ";";
	}
	evaluationDataOutputFile << avatar->getReached() << ";" << avatar->getStucked() << "\n";
	evaluationDataOutputFile.flush();
}

bool Logger::readLine(std::string str, Kore::vec3* rawPos, Kore::Quaternion* rawRot, float& scale, std::string& tag) {
	int column = 0;
	
	if (std::getline(logDataInputFile, str, '\n')) {
		std::stringstream ss;
		ss.str(str);
		std::string item;
		
		// Get tag
		std::getline(ss, item, ';');
		tag = item;
		
		while(std::getline(ss, item, ';')) {
			float num = std::stof(item);
			
			if (column == 0)		rawPos->x() = num;
			else if (column == 1)	rawPos->y() = num;
			else if (column == 2)	rawPos->z() = num;
			else if (column == 3)	rawRot->x = num;
			else if (column == 4)	rawRot->y = num;
			else if (column == 5)	rawRot->z = num;
			else if (column == 6)	rawRot->w = num;
			else if (column == 7)	scale = num;
			
			++column;
		}
		
		return true;
	}
	
	return false;
}

bool Logger::readData(const int numOfEndEffectors, const char* filename, Kore::vec3* rawPos, Kore::Quaternion* rawRot, float& scale) {
	std::string str;
	bool success = false;
	
	if (!logDataInputFile.is_open()) {
		logDataInputFile.open(filename);
		log(Kore::Info, "Read data from %s", filename);
	}
	
	// Skip header
	if(currLineNumber == 0) {
		std::getline(logDataInputFile, str, '\n');
		++currLineNumber;
	}
	
	// Read line
	for (int i = 0; i < numOfEndEffectors; ++i) {
		Kore::vec3 pos = Kore::vec3(0, 0, 0);
		Kore::Quaternion rot = Kore::Quaternion(0, 0, 0, 1);
		
		std::string tag;
		success = readLine(str, &pos, &rot, scale, tag);
		
		if (success) {
			++currLineNumber;
			rawPos[i] = pos;
			rawRot[i] = rot;
		}
	}
	
	if (logDataInputFile.eof()) {
		logDataInputFile.close();
		currLineNumber = 0;
	}
	
	return success;
}
