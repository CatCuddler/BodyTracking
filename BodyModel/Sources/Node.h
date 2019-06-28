#pragma once

class Node {
	
private:
	int data;
	Node* left;
	Node* right;
	
public:
	Node(int data);
	
	void setLeftNode(Node* node);
	void setRightNode(Node* node);
	
	int getData() const;
	Node* getLeft() const;
	Node* getRight() const;
};
