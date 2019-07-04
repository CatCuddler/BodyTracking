#pragma once

enum Character {
	Assistent, AssistentMagier, Clown1, Clown2, Clown3, Clown4, Dompteur, Dude, Magier, Zirkusdirektor, None
};

class Node {
	
private:
	const int ID;
	const char* const data;
	Character speakWithCharacter;
	
public:
	Node(int ID, Character speakWithCharacter, const char* const data);
	
	int getID() const;
	const char* getData() const;
	Character speakWith() const;
};
