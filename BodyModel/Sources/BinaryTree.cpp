#include "pch.h"
#include "BinaryTree.h"

#include <Kore/Log.h>

using namespace Kore;

BinaryTree::BinaryTree() {
	
	Node* root = new Node(0);
	Node* node1 = new Node(1);
	Node* node2 = new Node(2);
	
	
	root->setLeftNode(node1);
	root->setRightNode(node2);
	
	
	log(LogLevel::Info, "root: %i -> left %i -> right %i", root->getData(), root->getLeft()->getData(), root->getRight()->getData());
	
	log(LogLevel::Info, "left: %i -> left %i -> right %i", node1->getData(), node1->getLeft(), node1->getRight());
}
