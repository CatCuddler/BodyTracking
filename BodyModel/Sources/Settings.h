#pragma once

extern int ikMode;

struct EndEffector {
	Kore::vec3 offsetPosition;
	Kore::Quaternion offsetRotation;
	int boneIndex;
	int trackerIndex = -1;
	int ikMode; // 0: JT, 1: JPI, 2: DLS, 3: SVD, 4: DLS with SVD, 5: SDLS, 6: SDLS-Modified

	EndEffector(int boneIndex, int mode = 5) : boneIndex(boneIndex), ikMode(mode) {}
	
	void setTrackerIndex(int index) {
		trackerIndex = index;
	}
};

struct DataFile {
	const char* initialTransFilename;
	const char* positionDataFilename;
	bool isCalibrated; // todo: entfernen wenn alle alten Daten neu aufgezeichnet wurden
	
	DataFile(const char* init, const char* position, bool isCalibrated = true) : initialTransFilename(init), positionDataFilename(position), isCalibrated(isCalibrated) {}
	
	// todo: entfernen wenn alle alten Daten neu aufgezeichnet wurden
	void calibrated(bool state = true) {
		isCalibrated = state;
	}
};

namespace {
	EndEffector* tracker[] = {
		new EndEffector(17, ikMode), 	// left-hand // todo: oder 16?
		new EndEffector(27, ikMode), 	// right-hand // todo: oder 26?
		new EndEffector(2, ikMode), 	// back
		new EndEffector(6, ikMode),		// left-foot
		new EndEffector(31, ikMode), 	// right-foot
	};
	
	DataFile* calibrationFile = new DataFile("initTransAndRot_calibration.csv", "positionData_calibration.csv", false);
	DataFile* calibrationTestFile = new DataFile("initTransAndRot_calibration-test.csv", "positionData_calibration-test.csv", false);
	DataFile* joggingFile = new DataFile("initTransAndRot_jogging.csv", "positionData_jogging.csv", false);
	DataFile* kicksFile = new DataFile("initTransAndRot_kicks.csv", "positionData_kicks.csv", false);
	DataFile* squatsFile = new DataFile("initTransAndRot_squats.csv", "positionData_squats.csv", false);
	DataFile* walkingFile = new DataFile("initTransAndRot_walking.csv", "positionData_walking.csv", false);
	DataFile* currentFile = squatsFile;
	
	const float nearNull = 0.0001f;
	const int width = 1024;
	const int height = 768;
	const int rootIndex = 2;
	const bool renderTrackerAndController = true;
	const bool eval = true;
	const bool withOrientation = true;
	const float errorMaxPos = 0.01f;
	const float errorMaxRot = 0.01f;
}
