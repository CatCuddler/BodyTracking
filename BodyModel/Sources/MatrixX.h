#pragma once

#ifndef MATRIX_H
#define MATRIX_H

#include <vector>

class MatrixX {
protected:
	float* memory_;
	float** matrix_;
	unsigned int n_;
	unsigned int m_;
protected:
	void create(unsigned int n, unsigned int m);
	void destroy();
	bool ludcmp(std::vector<int>& indx, int* d);
	bool lubksb(std::vector<int>& indx, std::vector<float>& b);
public:
	MatrixX(unsigned int n = 0, unsigned int m = 0);
	MatrixX(const MatrixX& m);
	
	unsigned int getN() const { return n_; }
	unsigned int getM() const { return m_; }
	
	// Resize the matrix - destroys current contents
	void Resize(unsigned int n, unsigned int m) { destroy(); create(n, m); }
	
	void SetIdentity();
	
	void SetZero();
	
	void Transpose();
	
	// Invert an nxn matrix
	bool Invert();
	
	void Copy(const MatrixX& M);
	
	// this = A * B
	virtual bool Mult(const MatrixX& A, const MatrixX& B);
	
	// this = M * this
	bool PreMult(const MatrixX& M) { MatrixX tmp(*this); return Mult(M, tmp); }
	
	// this = this * M
	bool PostMult(const MatrixX& M) { MatrixX tmp(*this); return Mult(tmp, M); }
	
	// Assignment operators
	MatrixX& operator=(const MatrixX& M) { Copy(M); return *this; }
	MatrixX& operator*=(const MatrixX& M) { PostMult(M); return *this; }
	
	// Binary operator
	MatrixX operator*(const MatrixX& M) { MatrixX tmp; tmp.Mult(*this, M); return tmp; }
	
	float* operator[](int i) { return matrix_[i]; }
	const float* operator[](int i) const { return matrix_[i]; }
	
	virtual ~MatrixX();
};

#endif
