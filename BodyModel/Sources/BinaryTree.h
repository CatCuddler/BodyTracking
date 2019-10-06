#pragma once

#include "Node.h"

#include <Kore/Audio1/Sound.h>

class BinaryTree {
	
private:
	
	Node** nodes;
	int numOfNodes = 5152;
	
	int currentID;
	
	void createNewNode(int index, Character speakWithCharacter, const char* const data, Kore::Sound* audio);
	
public:
	BinaryTree();
	~BinaryTree();
	
	Node* getCurrentNode() const;
	bool setCurrentNode(int ID);
	Node* getLastNode() const;
	Node* getLeftNode() const;
	Node* getRightNode() const;
	Kore::Sound* getAudio() const;
};
