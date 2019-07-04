#pragma once

#include "Node.h"

class BinaryTree {
	
private:
	
	Node** nodes;
	int numOfNodes = 97;
	
	int currentID;
	
	void createNewNode(Character speakWithCharacter, const char* const data);
	
public:
	BinaryTree();
	~BinaryTree();
	
	Node* getCurrentNode() const;
	int getCurrentNodeID() const;
	Node* getLeftNode() const;
	Node* getRightNode() const;
	
	bool setCurrentNode(int ID);
};
