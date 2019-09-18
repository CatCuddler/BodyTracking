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

Yoga Movement::selectYogaPose(int random) {
	Yoga pose;
	switch (random) {
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


void Movement::getRandomMovement(Yoga& pose1) {
	int random = getRandom();
	pose1 = selectYogaPose(random);
	log(LogLevel::Info, "random pose %i", random);
}

void Movement::getRandomMovement(Yoga& pose1, Yoga& pose2) {
	int random1 = getRandom();
	pose1 = selectYogaPose(random1);
	
	int random2 = getRandom();
	while (random1 == random2) {
		random2 = getRandom();
	}
	pose2 = selectYogaPose(random2);
	
	log(LogLevel::Info, "random pose %i and %i", random1, random2);
}
