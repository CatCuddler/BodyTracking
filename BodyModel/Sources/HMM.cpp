#include "pch.h"
#include "HMM.h"

#include <Kore/Log.h>

#include <algorithm>

namespace {
	const char hmmPath[] = "../../HMM_Trainer/Tracking/";
	const char hmmPath0[] = "../../HMM_Trainer/Tracking/Movement0/";
	const char hmmPath1[] = "../../HMM_Trainer/Tracking/Movement1/";
	const char hmmPath2[] = "../../HMM_Trainer/Tracking/Movement2/";
	const char hmmName[] = "Yoga_Krieger";
	
	float currentUserHeight;
	
	int curentFileNumber = 0;
	int curentLineNumber = 0;
	
	const int numOfDataPoints = 6;
	int dataPointNumber; // x-th point of data collected in current recording/recognition
	
	// Vector of data points logged in real time movement recognition
	std::vector<std::vector<Point>> recognitionPoints(numOfDataPoints);
	
}

HMM::HMM(Logger& logger) : logger(logger), recording(false), recognizing(false) {
	curentFileNumber = 0;
}

bool HMM::isRecordingActive() {
	// Recording is only possible while there is no recognition in progress
	return (record && !recognition);
}

bool HMM::isRecognitionActive() {
	// Recognition is only possible while there is no recording in progress
	return (recognition && !record);
}

void HMM::init(Kore::vec3 hmdPosition, Kore::Quaternion hmdRotation) {
	currentUserHeight = hmdPosition.y();
	
	curentLineNumber = 0;
}

void HMM::startRecording(Kore::vec3 hmdPosition, Kore::Quaternion hmdRotation) {
	if (record) {
		init(hmdPosition, hmdRotation);
		logger.startHMMLogger(hmmName, curentFileNumber);
		curentFileNumber++;
	}
}

void HMM::stopRecording() {
	if (record) {
		logger.endHMMLogger();
	}
}

void HMM::startRecognition(Kore::vec3 hmdPosition, Kore::Quaternion hmdRotation) {
	if (recognition) {
		// Clear prievously stored points
		for (int i = 0; i < numOfDataPoints; i++) recognitionPoints.at(i).clear();
		dataPointNumber = 0;
		
		init(hmdPosition, hmdRotation);
	}
}

bool HMM::stopRecognition() {
	if (recognition) {
		// Read clusters for all trackers from file
		bool trackersPresent[numOfDataPoints];
		vector<KMeans> kmeanVector(numOfDataPoints);
		for (int ii = 0; ii < numOfDataPoints; ii++) {
			try {
				char hmmNameWithNum[50];
				sprintf(hmmNameWithNum, "%s_%i", hmmName, ii);
				KMeans kmeans(hmmPath, hmmNameWithNum);
				kmeanVector.at(ii) = kmeans;
				trackersPresent[ii] = true;
			} catch (std::invalid_argument) {
				trackersPresent[ii] = false;
				Kore::log(Kore::Error, "Can't find tracker file");
			}
		}
		// Store which trackers were recognised as the correct
		vector<bool> trackerMovementRecognised(numOfDataPoints, true);
		for (int ii = 0; ii < numOfDataPoints; ii++) {
			// Make sure the tracker is currently present and there is a HMM for it
			if (!recognitionPoints.at(ii).empty() && trackersPresent[ii]) {
				// Clustering data
				vector<int> clusteredPoints = kmeanVector.at(ii).matchPointsToClusters(normaliseMeasurements(recognitionPoints.at(ii), kmeanVector.at(ii).getAveragePoints()));
				// Reading HMM
				char hmmNameWithNum[50];
				sprintf(hmmNameWithNum, "%s_%i", hmmName, ii);
				HMMModel model(hmmPath, hmmNameWithNum);
				// Calculating the probability and comparing with probability threshold as well as applying restfix
				trackerMovementRecognised.at(ii) = (model.calculateProbability(clusteredPoints) > model.getProbabilityThreshold() && !std::equal(clusteredPoints.begin() + 1, clusteredPoints.end(), clusteredPoints.begin()));
				logger.analyseHMM(hmmName, model.calculateProbability(clusteredPoints), false);
			}
		}
		
		logger.analyseHMM(hmmName, 0, true);
		
		if (std::all_of(trackerMovementRecognised.begin(), trackerMovementRecognised.end(), [](bool v) { return v; })) {
			// All (present) trackers were recognised as correct
			return true;
		} else {
			return false;
		}
	}
	return false;
}

bool HMM::stopRecognitionAndIdentify() {
	if (recognition) {
		char hmmNameWithNum[50];
		// Read clusters for all trackers from file
		bool trackersPresent[numOfDataPoints];
		vector<KMeans> kmeanVector(numOfDataPoints);
		for (int ii = 0; ii < numOfDataPoints; ii++) {
			try {
				sprintf(hmmNameWithNum, "%s_%i", hmmName, ii);
				KMeans kmeans(hmmPath0, hmmNameWithNum);
				kmeanVector.at(ii) = kmeans;
				trackersPresent[ii] = true;
			} catch (std::invalid_argument) {
				trackersPresent[ii] = false;
				Kore::log(Kore::Error, "Can't find tracker file");
			}
		}
		// Store which trackers were recognised as the correct
		vector<bool> trackerMovementRecognised(numOfDataPoints, true);
		int n0=0;
		int n1=0;
		int n2=0;
		vector<int> clusteredPoints(numOfDataPoints);
		vector<bool>trackerMovementIdentify(numOfDataPoints, true);
		vector<double>probabilitymodel0(numOfDataPoints);
		vector<double>probabilitymodel1(numOfDataPoints);
		vector<double>probabilitymodel2(numOfDataPoints);
		vector<double>probabilityCurrentmovement(numOfDataPoints);
		// Create 3 HHModel for 3 different movement
		
		HMMModel model0(hmmPath0, hmmNameWithNum);
		HMMModel model1(hmmPath1, hmmNameWithNum);
		HMMModel model2(hmmPath2, hmmNameWithNum);
		for (int ii = 0; ii < numOfDataPoints; ii++) {
			// Make sure the tracker is currently present and there is a HMM for it
			if (!recognitionPoints.at(ii).empty() && trackersPresent[ii]) {
				// Clustering data
				clusteredPoints = kmeanVector.at(ii).matchPointsToClusters(normaliseMeasurements(recognitionPoints.at(ii), kmeanVector.at(ii).getAveragePoints()));
				// Reading HMM
				
				sprintf(hmmNameWithNum, "%s_%i", hmmName, ii);
				
				probabilitymodel0.at(ii)=model0.getProbabilityThreshold();
				probabilitymodel1.at(ii)=model1.getProbabilityThreshold();
				probabilitymodel2.at(ii)=model2.getProbabilityThreshold();
				probabilityCurrentmovement.at(ii)=model0.calculateProbability(clusteredPoints);
				n0 = abs(probabilityCurrentmovement.at(ii) - probabilitymodel0.at(ii));
				
				if(probabilityCurrentmovement.at(ii)>probabilitymodel1.at(ii)){
					n1++;
				}
				if(probabilityCurrentmovement.at(ii)>probabilitymodel2.at(ii)){
					n2++;
				}
			}
		}
		
		if (n0 > n1 && n0 > n2) {
			Kore::log(Kore::Info, "Recognize the movement as yoga warrior1");
			for (int ii = 0; ii < numOfDataPoints; ii++) {
				// Calculating the probability and comparing with probability threshold as well as applying restfix
				trackerMovementRecognised.at(ii) = (probabilityCurrentmovement.at(ii) > probabilitymodel0.at(ii) && !std::equal(clusteredPoints.begin() + 1, clusteredPoints.end(), clusteredPoints.begin()));
				logger.analyseHMM(hmmName, probabilityCurrentmovement.at(ii), false);
			}
		} else if (n1 > n0 && n1 > n2) {
			Kore::log(Kore::Info, "Recognize the movement as yoga warrior2");
			for (int ii = 0; ii < numOfDataPoints; ii++) {
				trackerMovementRecognised.at(ii) = (probabilityCurrentmovement.at(ii) > probabilitymodel1.at(ii) && !std::equal(clusteredPoints.begin() + 1, clusteredPoints.end(), clusteredPoints.begin()));
				logger.analyseHMM(hmmName, probabilityCurrentmovement.at(ii), false);
			}
		} else if (n2 > n1 && n2 > n0) {
			Kore::log(Kore::Info, "Recognize the movement as extended side angle");
			for (int ii = 0; ii < numOfDataPoints; ii++) {
				trackerMovementRecognised.at(ii) = (probabilityCurrentmovement.at(ii) > probabilitymodel2.at(ii) && !std::equal(clusteredPoints.begin() + 1, clusteredPoints.end(), clusteredPoints.begin()));
				logger.analyseHMM(hmmName, probabilityCurrentmovement.at(ii), false);
			}
		} else {
			Kore::log(Kore::Info, "None movement is recognized");
		}
		
		
		logger.analyseHMM(hmmName, 0, true);
		
		if (std::all_of(trackerMovementRecognised.begin(), trackerMovementRecognised.end(), [](bool v) { return v; })) {
			// All (present) trackers were recognised as correct
			return true;
		} else {
			return false;
		}
	}
	return false;
}

bool HMM::hmmActive() {
	if (recording || recognizing) return true;
	else return false;
}

bool HMM::hmmRecording() {
	return recording;
}

bool HMM::hmmRecognizing() {
	return recognizing;
}

void HMM::recordMovement(float lastTime, const char* name, Kore::vec3 position, Kore::Quaternion rotation) {
	// Either recording or recognition is active
	if (recording || recognizing) {
		
		curentLineNumber++;
		
		if (record) {
			// Data is recorded
			logger.saveHMMData(name, lastTime, position.normalize(), rotation);
		}
		
		if (recognition) { // data is stored internally for evaluation at the end of recognition
			double x, y, z, rotw, rotx, roty, rotz;
			// TODO: do we need to normalize position?
			x = position.x();
			y = position.y();
			z = position.z();
			rotw = rotation.w;
			rotx = rotation.x;
			roty = rotation.y;
			rotz = rotation.z;
			
			vector<double> values = { x, y, z, rotx, roty, rotz, rotw };
			Point point = Point(dataPointNumber, values);
			dataPointNumber++;
			
			if (std::strcmp(name, headTag) == 0)		recognitionPoints.at(0).push_back(point);
			else if (std::strcmp(name, lHandTag) == 0)	recognitionPoints.at(1).push_back(point);
			else if (std::strcmp(name, rHandTag) == 0)	recognitionPoints.at(2).push_back(point);
			else if (std::strcmp(name, hipTag) == 0)	recognitionPoints.at(3).push_back(point);
			else if (std::strcmp(name, lFootTag) == 0)	recognitionPoints.at(4).push_back(point);
			else if (std::strcmp(name, rFootTag) == 0)	recognitionPoints.at(5).push_back(point);
		}
	}
}
