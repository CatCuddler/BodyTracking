#pragma once

enum Yoga {
	Yoga1 = 0, Yoga2 = 1, Yoga3 = 2
};

class Movement {
	
private:
	Yoga lastMovement;
	
	int getRandom();
	
public:
	Movement();
	
	Yoga getRandomMovement();
};
