#include "pch.h"
#include "BinaryTree.h"

#include <Kore/Log.h>

using namespace Kore;

BinaryTree::BinaryTree() {
	
	const char* const text0 = "Deine Karriere läuft gut. Du arbeitest im Zirkus – ein Traum, den du schon von Kindheit an verfolgst... (1) Sprich mit deinem Assistenten (x) Sprich mit dem Zirkusdirektor";
	Node* root = new Node(None, text0);
	
	const char* const text1 = "(1) Sprich mit deinem Assistenten: Oh Gott es tut mir so leid! Das war alles meine Schuld! Ich muss was übersehen haben! Aber ich wüsste nicht was…. Ich hatte alles kontrolliert! Ich...ich schwör´s! Nach mir hat sogar der Dompteur noch die Seile Gecheckt!";
	Node* node1 = new Node(Assistent, text1);
	
	const char* const text2 = "(2) Sprich mit dem Dompteur: Ich war nicht an deinen Seilen dran. Warum sollte ich auch? Ich hab nach dem Lieblingsspielzeug von Schnuffel gesucht. Der arme Löwe kann ohne das Ding nicht schlafen. Einer der Clowns war auch da und hat mir beim Suchen geholfen.";
	Node* node2 = new Node(Dompteur, text2);
	
	
	// Test
	const char* const textX = "(x) Sprich mit dem Zirkusdirektor";
	Node* nodeX = new Node(Zirkusdirektor, textX);
	
	
	root->setLeftNode(node1);
	root->setRightNode(nodeX);
	node1->setLeftNode(node2);
	
	currentNode = root;
}

Node* BinaryTree::getCurrentNode() {
	return currentNode;
}

void BinaryTree::getLeftNode() {
	currentNode = currentNode->getLeft();
}

void BinaryTree::getRightNode() {
	currentNode = currentNode->getRight();
}
