#pragma once

#include "HMM.h"

class Movement {
	
private:
	//Yoga lastMovement;
	
	int getRandom();
	Yoga selectYogaPose(int random);
	
public:
	Movement();
	
	void getRandomMovement(Yoga& pose1);
	void getRandomMovement(Yoga& pose1, Yoga& pose2);
};
