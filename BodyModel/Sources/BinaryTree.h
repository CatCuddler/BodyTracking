#pragma once

#include "Node.h"

class BinaryTree {
	
private:
	
	Node* currentNode;
	
public:
	BinaryTree();
	
	Node* getCurrentNode();
	void getLeftNode();
	void getRightNode();
};
