#include "pch.h"

#include "HMM.h"

namespace {
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

/*void HMM::recordMovement(EndEffector* endEffector) {
	// Either recording or recognition is active
	if (record || recognition) {
		
		Kore::vec3 desiredPosition = endEffector->getDesPosition();
		
		transitionX = desiredPosition.x() - startX;
		transitionY = desiredPosition.y();
		transitionZ = desiredPosition.z() - startZ;
		
		if (record) {
			// Data is recorded
			Kore::vec3 hmmPos((transitionX * startRotCos - transitionZ * startRotSin), ((transitionY / currentUserHeight) * 1.8), (transitionZ * startRotCos + transitionX * startRotSin));
			// TODO: why dont we use raw data?
			logger->saveData(lastTime, identifier, hmmPos);
		}
		
		if (recognition) { // data is stored internally for evaluation at the end of recognition
			
			double x, y, z;
			x = (transitionX * startRotCos - transitionZ * startRotSin);
			y = (transitionY / currentUserHeight) * 1.8;
			z = (transitionZ * startRotCos + transitionX * startRotSin);
			
			vector<double> values = { x, y, z };
			Point point = Point(dataPointNumber, values);
			dataPointNumber++;
			
			const char* identifier = endEffector->getName();
			if (std::strcmp(identifier, headTag) == 0)			recognitionPoints.at(0).push_back(point);
			else if (std::strcmp(identifier, lHandTag) == 0)	recognitionPoints.at(1).push_back(point);
			else if (std::strcmp(identifier, rHandTag) == 0)	recognitionPoints.at(2).push_back(point);
			else if (std::strcmp(identifier, hipTag) == 0)		recognitionPoints.at(3).push_back(point);
			else if (std::strcmp(identifier, lFootTag) == 0)	recognitionPoints.at(4).push_back(point);
			else if (std::strcmp(identifier, rFootTag) == 0)	recognitionPoints.at(5).push_back(point);
		}
	}
}*/
