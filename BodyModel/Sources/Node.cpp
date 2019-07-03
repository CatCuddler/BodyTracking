#include "pch.h"
#include "Node.h"

using namespace Kore;

Node::Node(Character speakWithCharacter, const char* const data) : speakWithCharacter(speakWithCharacter), data(data), left(nullptr), right(nullptr) {
	
}

void Node::setLeftNode(Node* node) {
	left = node;
}

void Node::setRightNode(Node* node) {
	right = node;
}

const char* Node::getData() const {
	return data;
}

Character Node::speakWith() const {
	return speakWithCharacter;
}

Node* Node::getLeft() const {
	return left;
}

Node* Node::getRight() const {
	return right;
}
