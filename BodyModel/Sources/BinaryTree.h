#pragma once

#include "Node.h"

class BinaryTree {
	
private:
	
	Node* currentNode;
	
public:
	BinaryTree();
	
	Node* getCurrentNode();
	Node* getLeftNode();
	Node* getRightNode();
};
