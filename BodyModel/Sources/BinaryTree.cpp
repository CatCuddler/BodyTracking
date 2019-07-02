#include "pch.h"
#include "BinaryTree.h"

#include <Kore/Log.h>

using namespace Kore;

BinaryTree::BinaryTree() {
	
	const char* const text0 = "Deine Karriere läuft gut. Du arbeitest im Zirkus – ein Traum, den du schon von Kindheit an verfolgst.";
	Node* root = new Node(text0);
	const char* const text1 = "Sprich mit deinem Assistenten";
	Node* node1 = new Node(text1);
	const char* const text2 = "Oh Gott es tut mir so leid! Das war alles meine Schuld! Ich muss was übersehen haben! Aber ich wüsste nicht was…. Ich hatte alles kontrolliert! Ich…ich schwör´s! Nach mir hat sogar der Dompteur noch die Seile Gecheckt!";
	Node* node2 = new Node(text2);
	
	
	root->setLeftNode(node1);
	root->setRightNode(node2);

	node1->setLeftNode(nullptr);
	node1->setRightNode(nullptr);
	node2->setLeftNode(nullptr);
	node2->setRightNode(nullptr);
	
	currentNode = root;
}

Node* BinaryTree::getCurrentNode() {
	return currentNode;
}

Node* BinaryTree::getLeftNode() {
	currentNode = currentNode->getLeft();
	return currentNode;
}

Node* BinaryTree::getRightNode() {
	currentNode = currentNode->getRight();
	return currentNode;
}
