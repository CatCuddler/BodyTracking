#include "pch.h"
#include "Node.h"

using namespace Kore;

Node::Node(int data) : data(data), left(nullptr), right(nullptr) {
	
}

void Node::setLeftNode(Node* node) {
	left = node;
}

void Node::setRightNode(Node* node) {
	right = node;
}

int Node::getData() const {
	return data;
}

Node* Node::getLeft() const {
	return left;
}

Node* Node::getRight() const {
	return right;
}
