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
	int value = Random::get(0, 1);
	return value;
}

Yoga Movement::getRandomMovement() {
	int random = getRandom();
	Yoga movement = static_cast<Yoga>(random);
	log(LogLevel::Info, "random %i", random);
	return movement;
}
