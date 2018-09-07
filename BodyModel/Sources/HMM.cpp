#include "pch.h"

#include "HMM.h"

#include <Kore/Log.h>

namespace {
	std::string hmmPath = "../Tracking/";
	std::string hmmName = "Yoga_Krieger";
	
	// Initial tracked position as base for rotation of any futher data points
	double startX;
	double startZ;
	double startRotCos;
	double startRotSin;
	double transitionX;
	double transitionY;
	double transitionZ;
	int dataPointNumber; // x-th point of data collected in current recording/recognition
	
	// Vector of data points logged in real time movement recognition
	std::vector<std::vector<Point>> recognitionPoints(6);
	
}

HMM::HMM() {
	
}

void HMM::recordMovement(EndEffector& endEffector, float currentUserHeight) {
	// Either recording or recognition is active
	if (record || recognition) {
		
		Kore::vec3 desiredPosition = endEffector.getDesPosition();
		
		transitionX = desiredPosition.x() - startX;
		transitionY = desiredPosition.y();
		transitionZ = desiredPosition.z() - startZ;
		
		if (record) {
			// Data is recorded
			Kore::vec3 hmmPos((transitionX * startRotCos - transitionZ * startRotSin), ((transitionY / currentUserHeight) * 1.8), (transitionZ * startRotCos + transitionX * startRotSin));
			// TODO: why dont we use calibrated data? Why do we need 1.8?
//			logger->saveData(lastTime, identifier, hmmPos);
		}
		
		if (recognition) { // data is stored internally for evaluation at the end of recognition
			
			double x, y, z;
			x = (transitionX * startRotCos - transitionZ * startRotSin);
			y = (transitionY / currentUserHeight) * 1.8;
			z = (transitionZ * startRotCos + transitionX * startRotSin);
			
			vector<double> values = { x, y, z };
			Point point = Point(dataPointNumber, values);
			dataPointNumber++;
			
			const char* identifier = endEffector.getName();
			if (std::strcmp(identifier, headTag) == 0)			recognitionPoints.at(0).push_back(point);
			else if (std::strcmp(identifier, lHandTag) == 0)	recognitionPoints.at(1).push_back(point);
			else if (std::strcmp(identifier, rHandTag) == 0)	recognitionPoints.at(2).push_back(point);
			else if (std::strcmp(identifier, hipTag) == 0)		recognitionPoints.at(3).push_back(point);
			else if (std::strcmp(identifier, lFootTag) == 0)	recognitionPoints.at(4).push_back(point);
			else if (std::strcmp(identifier, rFootTag) == 0)	recognitionPoints.at(5).push_back(point);
		}
	}
}

void HMM::startRecognition(EndEffector& hmd) {
	// Clear prievously stored points
	recognitionPoints.at(0).clear();
	recognitionPoints.at(1).clear();
	recognitionPoints.at(2).clear();
	recognitionPoints.at(3).clear();
	recognitionPoints.at(4).clear();
	recognitionPoints.at(5).clear();
	dataPointNumber = 0;
	
	// Save current HMD position and rotation for data normalisation
	Kore::vec3 hmdPosition = hmd.getDesPosition();
	Kore::Quaternion hmdRotation = hmd.getDesRotation();
	startX = hmdPosition.x();
	startZ = hmdPosition.z();
	startRotCos = Kore::cos(hmdRotation.y * Kore::pi);
	startRotSin = Kore::sin(hmdRotation.y * Kore::pi);
}

bool HMM::stopRecognition() {
	// Read clusters for all trackers from file
	bool trackersPresent[6];
	vector<KMeans> kmeanVector(6);
	for (int ii = 0; ii < 6; ii++) {
		try {
			KMeans kmeans(hmmPath, hmmName + "_" + to_string(ii));
			kmeanVector.at(ii) = kmeans;
			trackersPresent[ii] = true;
		}
		catch (std::invalid_argument) {
			trackersPresent[ii] = false;
			Kore::log(Kore::Info, "can't find tracker file");
		}
	}
	vector<bool> trackerMovementRecognised(6, true); // store which trackers were recognised as the correct movement
	for (int ii = 0; ii < 6; ii++) { // check all trackers
		if (!recognitionPoints.at(ii).empty() && trackersPresent[ii]) { // make sure the tracker is currently present and there is a HMM for it
			vector<int> clusteredPoints = kmeanVector.at(ii).matchPointsToClusters(normaliseMeasurements(recognitionPoints.at(ii), kmeanVector.at(ii).getAveragePoints())); // clustering data
			HMMModel model(hmmPath, hmmName + "_" + to_string(ii)); // reading HMM
			trackerMovementRecognised.at(ii) = (model.calculateProbability(clusteredPoints) > model.getProbabilityThreshold() && !std::equal(clusteredPoints.begin() + 1, clusteredPoints.end(), clusteredPoints.begin())); // calculating probability and comparing with probability threshold as well as applying restfix
//			logger->analyseHMM(hmmName.c_str(), model.calculateProbability(clusteredPoints), false);
		}
	}
	
	//logger->analyseHMM(hmmName.c_str(), 0, true);
	
	if (std::all_of(trackerMovementRecognised.begin(), trackerMovementRecognised.end(), [](bool v) { return v; })) { // all (present) trackers were recognised as correct
		Kore::log(Kore::Info, "The movement is correct");
		return true;
	} else {
		Kore::log(Kore::Info, "The movement is wrong");
		return false;
	}
	return false;
}
