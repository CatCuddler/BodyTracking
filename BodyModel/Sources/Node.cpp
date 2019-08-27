#include "pch.h"
#include "Node.h"

using namespace Kore;

Node::Node(int ID, Character speakWithCharacter, const char* const data, Sound* audio) : ID(ID), speakWithCharacter(speakWithCharacter), data(data), audio(audio) {
	
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

Sound* Node::getAudio() const {
	return audio;
}
