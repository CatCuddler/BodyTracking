#pragma once

#include <Kore/Math/Vector.h>
#include <Kore/Math/Quaternion.h>

class EndEffector {
public:
	EndEffector(int boneIndex, int mode = 5);
	
	void setTrackerIndex(int index);
	int getBoneIndex() const;
	
private:
	Kore::vec3 offsetPosition;
	Kore::Quaternion offsetRotation;
	int boneIndex;
	int trackerIndex;
	int ikMode; // 0: JT, 1: JPI, 2: DLS, 3: SVD, 4: DLS with SVD, 5: SDLS, 6: SDLS-Modified
};
