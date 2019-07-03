#pragma once

enum Character {
	Assistent, AssistentMagier, Clown1, Clown2, Clown3, Clown4, Dompteur, Dude, Magier, Zirkusdirektor, None
};

class Node {
	
private:
	const char* const data;
	Character speakWithCharacter;
	Node* left;
	Node* right;
	
public:
	Node(Character speakWithCharacter, const char* const data);
	
	void setLeftNode(Node* node);
	void setRightNode(Node* node);
	
	const char* getData() const;
	Character speakWith() const;
	Node* getLeft() const;
	Node* getRight() const;
};
