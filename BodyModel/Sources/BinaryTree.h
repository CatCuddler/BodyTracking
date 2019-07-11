#pragma once

#include "Node.h"

class BinaryTree {
	
private:
	
	Node** nodes;
	int numOfNodes = 5152;
	
	int currentID;
	
	void createNewNode(int index, Character speakWithCharacter, const char* const data);
	
public:
	BinaryTree();
	~BinaryTree();
	
	Node* getCurrentNode() const;
	Node* getLastNode() const;
	int getCurrentNodeID() const;
	Node* getLeftNode() const;
	Node* getRightNode() const;
	
	bool setCurrentNode(int ID);
};
