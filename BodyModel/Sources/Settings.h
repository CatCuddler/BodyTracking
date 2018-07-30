#pragma once

struct EndEffector {
	Kore::vec3 offsetPosition;
	Kore::Quaternion offsetRotation;
	int boneIndex;
	int trackerIndex = -1;
	int ikMode; // 0: JT, 1: JPI, 2: DLS, 3: SVD, 4: DLS with SVD, 5: SDLS
	
	EndEffector(int boneIndex,
				Kore::vec3 offsetPosition = Kore::vec3(0, 0, 0),
				Kore::Quaternion offsetRotation = Kore::Quaternion(0, 0, 0, 1),
				int ikMode = 4
	) :
	boneIndex(boneIndex),
	offsetPosition(offsetPosition),
	offsetRotation(offsetRotation),
	ikMode(ikMode)
	{}
	
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
	void calibrated() {
		isCalibrated = true;
	}
};

namespace {
	const int ikMode = 5; // 0: JT, 1: JPI, 2: DLS, 3: SVD, 4: DLS with SVD, 5: SDLS
	const bool withOrientation = true;
	const int maxSteps = 100;
	const float errorMax = 0.01f;
	const float lambda[] = { -1.0f, -1.0f, 0.18f, -1.0f, 0.18f, 0.7853981634f };
	
	EndEffector* tracker[] = {
		new EndEffector(16,
						Kore::vec3(0.020000, 0.020000, 0.000000),
						Kore::Quaternion(-0.500000, 0.321020, -0.630037, 0.500000),
						ikMode
						), // left-hand
		new EndEffector(26,
						Kore::vec3(-0.020000, 0.020000, 0.000000),
						Kore::Quaternion(-0.500000, -0.321020, 0.630037, 0.500000),
						ikMode
						), // right-hand
		new EndEffector(2,
						Kore::vec3(0.000000, 0.000000, 0.000000),
						Kore::Quaternion(0.000000, 0.125333, -0.992115, -0.000000),
						ikMode
						), // back
		new EndEffector(6,
						Kore::vec3(0.050000, 0.000000, 0.000000),
						Kore::Quaternion(0.698401, 0.110616, -0.698401, -0.110616),
						ikMode
						),	// left-foot
		new EndEffector(31,
						Kore::vec3(0.050000, 0.000000, 0.000000),
						Kore::Quaternion(0.698401, -0.110616, 0.698401, -0.110616),
						ikMode
						), // right-foot
	};
	
	DataFile* calibrationFile = new DataFile("initTransAndRot_calibration.csv", "positionData_calibration.csv", false);
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
}
