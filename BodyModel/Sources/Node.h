#pragma once

#include <Kore/Audio1/Sound.h>

enum Character {
	Assistent, AssistentMagier, Clown1, Clown2, Clown3, Clown4, Dompteur, Dude, Magier, Zirkusdirektor, None
};

class Node {
	
private:
	const int ID;
	const char* const data;
	Character speakWithCharacter;
	Kore::Sound* audio;
	
public:
	Node(int ID, Character speakWithCharacter, const char* const data, Kore::Sound* audio);
	
	int getID() const;
	const char* getData() const;
	Character speakWith() const;
	Kore::Sound* getAudio() const;
};
