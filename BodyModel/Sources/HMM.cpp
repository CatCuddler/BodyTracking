#include "pch.h"
#include "HMM.h"

#include <Kore/Log.h>

#include <algorithm>

namespace {
	const char hmmPath[] = "../../HMM_Trainer/Tracking/";
	const char hmmName[] = "Yoga_Krieger";
	
	// Initial tracked position as base for rotation of any futher data points
	double startX;
	double startZ;
	double startRotCos;
	double startRotSin;
	double transitionX;
	double transitionY;
	double transitionZ;
	
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
	// Save current HMD position and rotation for data normalisation
	startX = hmdPosition.x();
	startZ = hmdPosition.z();
	startRotCos = Kore::cos(hmdRotation.y * Kore::pi);
	startRotSin = Kore::sin(hmdRotation.y * Kore::pi);
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

		transitionX = position.x() - startX;
		transitionY = position.y();
		transitionZ = position.z() - startZ;
		
		if (record) {
			// Data is recorded
			// TODO: Why do we need 1.8?
			//Kore::vec3 hmmPos((transitionX * startRotCos - transitionZ * startRotSin), ((transitionY / currentUserHeight) * 1.8), (transitionZ * startRotCos + transitionX * startRotSin));
			//logger.saveHMMData(name, lastTime, hmmPos, rotation);
			
			logger.saveHMMData(name, lastTime, position.normalize(), rotation);
		}
		
		if (recognition) { // data is stored internally for evaluation at the end of recognition
			
			double x, y, z;
			x = (transitionX * startRotCos - transitionZ * startRotSin);
			y = (transitionY / currentUserHeight) * 1.8;
			z = (transitionZ * startRotCos + transitionX * startRotSin);
			
			vector<double> values = { x, y, z };
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
