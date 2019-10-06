#pragma once

#include "HMM.h"

class Movement {
	
private:
	//Yoga lastMovement;
	
	int getRandom();
	Yoga selectYogaPose(int ID);
	
public:
	Movement();
	
	void getRandomMovement(Yoga& pose0, Yoga currentPose);
	void getRandomMovement(Yoga& pose0, Yoga& pose1, Yoga currentPose);
};
