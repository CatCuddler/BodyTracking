#include "pch.h"
#include "YogaMovement.h"

#include <Kore/System.h>
#include <Kore/Log.h>
#include <Kore/Math/Random.h>

using namespace Kore;

Movement::Movement() {
	Random::init(static_cast<int>(System::time() * 1000));
}

int Movement::getRandom() {
	int value = Random::get(0, 2);
	return value;
}

Yoga Movement::selectYogaPose(int ID) {
	Yoga pose;
	switch (ID) {
		case 0:
			pose = Yoga0;
			break;
			
		case 1:
			pose = Yoga1;
			break;
			
		case 2:
			pose = Yoga2;
			break;
			
		default:
			pose = Unknown;
			break;
	}
	
	return pose;
}


void Movement::getRandomMovement(Yoga& pose0, Yoga currentPose) {
	int random = getRandom();
	pose0 = selectYogaPose(random);
	while (pose0 == currentPose) {
		random = getRandom();
		pose0 = selectYogaPose(random);
	}
	log(LogLevel::Info, "random pose %i", random);
}

void Movement::getRandomMovement(Yoga& pose0, Yoga& pose1, Yoga currentPose) {
	getRandomMovement(pose0, currentPose);
	getRandomMovement(pose1, pose0);
	while (pose1 == currentPose) {
		getRandomMovement(pose1, pose0);
	}
	
	log(LogLevel::Info, "random pose %i and %i", pose0, pose1);
}
