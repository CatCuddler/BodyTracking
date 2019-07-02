#pragma once

class Node {
	
private:
	const char* const data;
	Node* left;
	Node* right;
	
public:
	Node(const char* const data);
	
	void setLeftNode(Node* node);
	void setRightNode(Node* node);
	
	const char* getData() const;
	Node* getLeft() const;
	Node* getRight() const;
};
