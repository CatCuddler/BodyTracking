#include "pch.h"
#include "Node.h"

using namespace Kore;

Node::Node(int ID, Character speakWithCharacter, const char* const data) : ID(ID), speakWithCharacter(speakWithCharacter), data(data) {
	
}

int Node::getID() const {
	return ID;
}

const char* Node::getData() const {
	return data;
}

Character Node::speakWith() const {
	return speakWithCharacter;
}
