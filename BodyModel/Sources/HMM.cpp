#include "pch.h"
#include "HMM.h"

#include <Kore/Log.h>

#include <algorithm>
#include <iterator>
#include <valarray>



namespace {
	const char hmmPath[] = "../../HMM_Trainer/Tracking/";
	const char hmmPath0[] = "../../HMM_Trainer/Tracking/Movement0/";
	const char hmmPath1[] = "../../HMM_Trainer/Tracking/Movement1/";
	const char hmmPath2[] = "../../HMM_Trainer/Tracking/Movement2/";
	const char hmmName[] = "Yoga_Krieger";

	float threshold = 1.5f;

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

	bool hmm_head = false;
	bool hmm_hip = false;
	bool hmm_leftArm = false;
	bool hmm_rightArm = false;
	bool hmm_leftLeg = false;
	bool hmm_rightLeg = false;

	float hmm_head_modelProbability = 1.0f;
	float hmm_hip_modelProbability = 1.0f;
	float hmm_leftArm_modelProbability = 1.0f;
	float hmm_rightArm_modelProbability = 1.0f;
	float hmm_leftLeg_modelProbability = 1.0f;
	float hmm_rightLeg_modelProbability = 1.0f;

	float hmm_head_modelThreshold = 1.0f;
	float hmm_hip_modelThreshold = 1.0f;
	float hmm_leftArm_modelThreshold = 1.0f;
	float hmm_rightArm_modelThreshold = 1.0f;
	float hmm_leftLeg_modelThreshold = 1.0f;
	float hmm_rightLeg_modelThreshold = 1.0f;
}

HMM::HMM(Logger& logger) : logger(logger) {
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

bool HMM::stopRecognition(const char* path123) {
	if (recognition) {
		// Read clusters for all trackers from file
		bool trackersPresent[numOfDataPoints];
		vector<KMeans> kmeanVector(numOfDataPoints);
		for (int ii = 0; ii < numOfDataPoints; ii++) {
			try {
				char hmmNameWithNum[50];
				sprintf(hmmNameWithNum, "%s_%i", hmmName, ii);
				KMeans kmeans(path123, hmmNameWithNum);
				kmeanVector.at(ii) = kmeans;
				trackersPresent[ii] = true;
			}
			catch (std::invalid_argument) {
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
				HMMModel model(path123, hmmNameWithNum);
				// Calculating the probability and comparing with probability threshold as well as applying restfix
				float modelProbability = model.calculateProbability(clusteredPoints);
				float modelThreshold = model.getProbabilityThreshold() * threshold;
				trackerMovementRecognised.at(ii) = (modelProbability > modelThreshold);
				if (ii == 0) {
					hmm_head = trackerMovementRecognised.at(ii);
					Kore::log(Kore::LogLevel::Info, "Probability for head: (%f,%f) --> %s", modelProbability, modelThreshold, trackerMovementRecognised.at(ii) ? "true" : "false");
					hmm_head_modelProbability = modelProbability;
					hmm_head_modelThreshold = modelThreshold;
				} else if (ii == 1) {
					hmm_leftArm = trackerMovementRecognised.at(ii);
					Kore::log(Kore::LogLevel::Info, "Probability for left arm: (%f,%f) --> %s", modelProbability, modelThreshold, trackerMovementRecognised.at(ii) ? "true" : "false");
					hmm_leftArm_modelProbability = modelProbability;
					hmm_leftArm_modelThreshold = modelThreshold;
				} else if (ii == 2) {
					hmm_rightArm = trackerMovementRecognised.at(ii);
					Kore::log(Kore::LogLevel::Info, "Probability for right Arm: (%f,%f) --> %s", modelProbability, modelThreshold, trackerMovementRecognised.at(ii) ? "true" : "false");
					hmm_rightArm_modelProbability = modelProbability;
					hmm_rightArm_modelThreshold = modelThreshold;
				} else if (ii == 3) {
					hmm_hip = trackerMovementRecognised.at(ii);
					Kore::log(Kore::LogLevel::Info, "Probability for hip: (%f,%f) --> %s", modelProbability, modelThreshold, trackerMovementRecognised.at(ii) ? "true" : "false");
					hmm_hip_modelProbability = modelProbability;
					hmm_hip_modelThreshold = modelThreshold;
				} else if (ii == 4) {
					hmm_leftLeg = trackerMovementRecognised.at(ii);
					Kore::log(Kore::LogLevel::Info, "Probability for left foot: (%f,%f) --> %s", modelProbability, modelThreshold, trackerMovementRecognised.at(ii) ? "true" : "false");
					hmm_leftLeg_modelProbability = modelProbability;
					hmm_leftLeg_modelThreshold = modelThreshold;
				} else if (ii == 5) {
					hmm_rightLeg = trackerMovementRecognised.at(ii);
					Kore::log(Kore::LogLevel::Info, "Probability for right foot: (%f,%f) --> %s", modelProbability, modelThreshold, trackerMovementRecognised.at(ii) ? "true" : "false");
					hmm_rightLeg_modelProbability = modelProbability;
					hmm_rightLeg_modelThreshold = modelThreshold;
				}

				logger.analyseHMM(hmmName, model.calculateProbability(clusteredPoints), false);
			}
		}

		logger.analyseHMM(hmmName, 0, true);

		// Ignore head
		if (std::all_of(trackerMovementRecognised.begin() + 1, trackerMovementRecognised.end(), [](bool v) { return v; })) {
			// All (present) trackers were recognised as correct
			return true;
		} else {
			return false;
		}
	}
	return false;
}

bool HMM::stopRecognitionAndIdentify(Yoga yogaPos) {
	bool recognized = false;
	switch (yogaPos) {
		case Yoga0:
			recognized = stopRecognition(hmmPath0);
			break;
		case Yoga1:
			recognized = stopRecognition(hmmPath1);
			break;
		case Yoga2:
			recognized = stopRecognition(hmmPath2);
			break;
		default:
			break;
	}
	return recognized;
}

bool HMM::stopRecognitionAndIdentify() {
	if (recognition) {
		// Read clusters for all trackers from file
		bool trackersPresent[numOfDataPoints];
		vector<vector<KMeans>> kmeanVector(3, vector <KMeans>(numOfDataPoints));
		for (int ii = 0; ii < numOfDataPoints; ii++) {
			try {
				char hmmNameWithNum[50];
				sprintf(hmmNameWithNum, "%s_%i", hmmName, ii);
				KMeans kmeans0(hmmPath0, hmmNameWithNum);
				KMeans kmeans1(hmmPath1, hmmNameWithNum);
				KMeans kmeans2(hmmPath2, hmmNameWithNum);
				kmeanVector.at(0).at(ii) = kmeans0;
				kmeanVector.at(1).at(ii) = kmeans1;
				kmeanVector.at(2).at(ii) = kmeans2;
				trackersPresent[ii] = true;
			} catch (std::invalid_argument) {
				trackersPresent[ii] = false;
				Kore::log(Kore::Error, "Can't find tracker file");
			}
		}
		// Store which trackers were recognised as the correct
		vector<bool> trackerMovementRecognised(numOfDataPoints, true);
		
		vector<int>identifyProbability;
		vector<double>probabilityModel0;
		vector<double>probabilityModel1;
		vector<double>probabilityModel2;
		vector<double>probabilityStandardModel0;
		vector<double>probabilityStandardModel1;
		vector<double>probabilityStandardModel2;
		double ps0, ps1, ps2;
		// Create 3 HHModel for 3 different movement
		for (int ii = 0; ii < numOfDataPoints; ii++) {
			// Make sure the tracker is currently present and there is a HMM for it
			if (!recognitionPoints.at(ii).empty() && trackersPresent[ii]) {
				// Clustering data
				vector<vector<int>>clusteredPoints(3, vector <int>());
				clusteredPoints.at(0) = kmeanVector.at(0).at(ii).matchPointsToClusters(normaliseMeasurements(recognitionPoints.at(ii), kmeanVector.at(0).at(ii).getAveragePoints()));
				clusteredPoints.at(1) = kmeanVector.at(1).at(ii).matchPointsToClusters(normaliseMeasurements(recognitionPoints.at(ii), kmeanVector.at(1).at(ii).getAveragePoints()));
				clusteredPoints.at(2) = kmeanVector.at(2).at(ii).matchPointsToClusters(normaliseMeasurements(recognitionPoints.at(ii), kmeanVector.at(2).at(ii).getAveragePoints()));
				// Reading HMM
				char hmmNameWithNum[50];
				sprintf(hmmNameWithNum, "%s_%i", hmmName, ii);
				HMMModel model0(hmmPath0, hmmNameWithNum);
				HMMModel model1(hmmPath1, hmmNameWithNum);
				HMMModel model2(hmmPath2, hmmNameWithNum);
				float p0 = model0.calculateProbability(clusteredPoints.at(0));
				float p1 = model1.calculateProbability(clusteredPoints.at(1));
				float p2 = model2.calculateProbability(clusteredPoints.at(2));
				ps0 = model0.getProbabilityThreshold() * threshold;
				ps1 = model1.getProbabilityThreshold() * threshold;
				ps2 = model2.getProbabilityThreshold() * threshold;
				vector<double> probabilityCurrentmovement = { p0, p1, p2 };
				probabilityModel0.push_back(p0);
				probabilityModel1.push_back(p1);
				probabilityModel2.push_back(p2);
				probabilityStandardModel0.push_back(ps0);
				probabilityStandardModel1.push_back(ps1);
				probabilityStandardModel2.push_back(ps2);
				Kore::log(Kore::LogLevel::Info, "model0: (%f,%f)", model0.calculateProbability(clusteredPoints.at(0)), model0.getProbabilityThreshold());
				Kore::log(Kore::LogLevel::Info, "model1: (%f,%f)", model1.calculateProbability(clusteredPoints.at(1)), model1.getProbabilityThreshold());
				Kore::log(Kore::LogLevel::Info, "model2: (%f,%f)", model2.calculateProbability(clusteredPoints.at(2)), model2.getProbabilityThreshold());

				std::vector<double>::iterator biggest = std::max_element(std::begin(probabilityCurrentmovement), std::end(probabilityCurrentmovement));
				//	Kore::log(Kore::LogLevel::Info, "max: (%f)", probabilityCurrentmovement.end());
				identifyProbability.push_back(std::distance(std::begin(probabilityCurrentmovement), biggest));
			}
		}
		int n0, n1, n2 = -1;
		n0 = count(identifyProbability.begin(), identifyProbability.end(), 0);
		n1 = count(identifyProbability.begin(), identifyProbability.end(), 1);
		n2 = count(identifyProbability.begin(), identifyProbability.end(), 2);
		logger.analyseHMM(hmmName, 0, true);
		Kore::log(Kore::LogLevel::Info, "Probability: (%d,%d,%d)", n0, n1, n2);
		if (n0 > n1 && n0 > n2) {
			log(Kore::Info, "Current movement is recognized as Pose 1.");
			for (int ii = 0; ii < numOfDataPoints; ii++) {
				trackerMovementRecognised.at(ii) = (probabilityModel0.at(ii) > probabilityStandardModel0.at(ii));
				identifiedYogaPose = Yoga0;
			}
		}
		else if (n1 > n0 && n1 > n2) {
			log(Kore::Info, "Current movement is recognized as Pose 2.");
			for (int ii = 0; ii < numOfDataPoints; ii++) {
				trackerMovementRecognised.at(ii) = (probabilityModel1.at(ii) > probabilityStandardModel1.at(ii));
				identifiedYogaPose = Yoga1;
			}
		} else if (n2 > n0 && n2 > n1) {
			log(Kore::Info, "Current movement is recognized as Pose 3.");
			for (int ii = 0; ii < numOfDataPoints; ii++) {
				trackerMovementRecognised.at(ii) = (probabilityModel2.at(ii) > probabilityStandardModel2.at(ii));
				identifiedYogaPose = Yoga2;
			}
		} else {
			log(Kore::Info, "None movement is recognized");
		}

		// Ignore HMD (+1)
		if (std::all_of(trackerMovementRecognised.begin() + 1, trackerMovementRecognised.end(), [](bool v) { return v; })) {
			// All (present) trackers were recognised as correct
			return true;
		} else {
			return false;
		}
	}
	return false;
}

void HMM::recordMovement(float lastTime, const char* name, Kore::vec3 position, Kore::Quaternion rotation) {
	curentLineNumber++;

	transitionX = position.x() - startX;
	transitionY = position.y();
	transitionZ = position.z() - startZ;
	if (record) {
		// Data is recorded
		logger.saveHMMData(name, lastTime, position.normalize(), rotation);
	}

	if (recognition) { // data is stored internally for evaluation at the end of recognition
		double x, y, z, rotw, rotx, roty, rotz;
		// TODO: do we need to normalize position?
		x = position.normalize().x();
		y = position.normalize().y();
		z = position.normalize().z();
		rotw = rotation.w;
		rotx = rotation.x;
		roty = rotation.y;
		rotz = rotation.z;
		//			x = (transitionX * startRotCos - transitionZ * startRotSin);
		//			y = (transitionY / currentUserHeight) * 1.8;
		//			z = (transitionZ * startRotCos + transitionX * startRotSin);

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

void HMM::getFeedback(bool &head, bool &hip, bool &leftArm, bool &rightArm, bool &leftLeg, bool &rightLeg) {
	head = hmm_head;
	hip = hmm_hip;
	leftArm = hmm_leftArm;
	rightArm = hmm_rightArm;
	leftLeg = hmm_leftLeg;
	rightLeg = hmm_rightLeg;
}

void HMM::getFeedbackModel(float& head_prob, float& hip_prob, float& leftArm_prob, float& rightArm_prob, float& leftLeg_prob, float& rightLeg_prob, float& head, float& hip, float& leftArm, float& rightArm, float& leftLeg, float& rightLeg) {
	head_prob = hmm_head_modelProbability;
	hip_prob = hmm_hip_modelProbability;
	leftArm_prob = hmm_leftArm_modelProbability;
	rightArm_prob = hmm_rightArm_modelProbability;
	leftLeg_prob = hmm_leftLeg_modelProbability;
	rightLeg_prob = hmm_rightLeg_modelProbability;
	head = hmm_head_modelThreshold;
	hip = hmm_hip_modelThreshold;
	leftArm = hmm_leftArm_modelThreshold;
	rightArm = hmm_rightArm_modelThreshold;
	leftLeg = hmm_leftLeg_modelThreshold;
	rightLeg = hmm_rightLeg_modelThreshold;
}