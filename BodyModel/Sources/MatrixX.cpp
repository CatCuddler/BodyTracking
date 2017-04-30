#include "pch.h"
#include "MatrixX.h"

#include <Kore/Math/Core.h>

MatrixX::MatrixX(unsigned int n, unsigned int m) {
	create(n, m);
	SetZero();
}

MatrixX::MatrixX(const MatrixX& M) {
	unsigned int i, count = M.n_ * M.m_;
	create(M.n_, M.m_);
	for(i = 0; i < count; i++)
		memory_[i] = M.memory_[i];
}

void MatrixX::create(unsigned int n, unsigned int m) {
	unsigned int i;
	
	if(n != 0 && m != 0) {
		memory_ = new float[n*m];
		matrix_ = new float*[n];
		for(i = 0; i < n; i++)
			matrix_[i] = &memory_[i * m];
	}
	
	n_ = n;
	m_ = m;
}

void MatrixX::destroy() {
	if(n_ != 0 && m_ != 0) {
		delete [] matrix_;
		delete [] memory_;
	}
	
	n_ = 0;
	m_ = 0;
}

void MatrixX::Copy(const MatrixX& M) {
	unsigned int i, count = M.n_ * M.m_;
	
	destroy();
	
	create(M.n_, M.m_);
	for(i = 0; i < count; i++)
		memory_[i] = M.memory_[i];
}

void MatrixX::SetIdentity() {
	unsigned int i, j;
	
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++)
			matrix_[i][j] = (i == j) ? 1.0 : 0.0;
	}
}

void MatrixX::SetZero() {
	unsigned int i, j;
	
	for (i = 0; i < n_; i++) {
		for (j = 0; j < m_; j++)
			matrix_[i][j] = 0.0;
	}
}

#define TINY 1.0e-20
bool MatrixX::ludcmp(std::vector<int>& indx, int* d) {
	int i, imax, j, k;
	float big, dum, sum, temp;
	
	// Matrix must be square
	if(n_ != m_)
		return false;
	
	// Create an array of the appropriate size
	std::vector<float> vv(n_);
	
	// Ensure the array is the appropriate size
	indx.resize(n_);
	
	*d = 1;
	
	for(i = 0; i < n_; i++)
		indx[i] = i;
	
	for(i = 0; i < n_; i++) {
		big = 0.0;
		for(j = 0; j < n_; j++) {
			if((temp = Kore::abs(matrix_[i][j])) > big)
				big = temp;
		}
		
		// Matrix most not be singular
		if (big == 0.0)
			return false;
		
		vv[i] = 1.0 / big;
	}
	
	for(j = 0; j < n_; j++) {
		for(i = 0; i < j; i++) {
			sum = matrix_[i][j];
			
			for(k = 0; k < i; k++)
				sum -= matrix_[i][k] * matrix_[k][j];
			
			matrix_[i][j] = sum;
		}
		
		big = 0.0;
		imax = -1;
		for(i = j; i < n_; i++) {
			sum = matrix_[i][j];
			
			for (k = 0; k < j; k++)
				sum -= matrix_[i][k] * matrix_[k][j];
			
			matrix_[i][j] = sum;
			
			if((dum = vv[i] * Kore::abs(sum)) >= big) {
				big = dum;
				imax = i;
			}
		}
		
		if(j != imax) {
			for(k = 0; k < n_; k++) {
				dum = matrix_[imax][k];
				matrix_[imax][k] = matrix_[j][k];
				matrix_[j][k] = dum;
			}
			
			*d = -(*d);
			vv[imax] = vv[j];
		}
		
		indx[j] = imax;
		
		if(matrix_[j][j] == 0.0)
			matrix_[j][j] = TINY;
		
		dum = 1.0 / matrix_[j][j];
		
		for(i = j + 1; i < n_; i++)
			matrix_[i][j] *= dum;
	}
	
	return true;
}

bool MatrixX::lubksb(std::vector<int>& indx, std::vector<float>& b) {
	int i, ii = -1, ip, j;
	float sum;
	
	// Matrix must be square
	if(n_ != m_)
		return false;
	
	for(i = 0; i < n_; i++) {
		ip = indx[i];
		sum = b[ip];
		b[ip] = b[i];
		
		if(ii != -1) {
			for(j = ii; j < i; j++)
				sum -= matrix_[i][j] * b[j];
		}
		else if(sum != 0.0)
			ii=i;
		
		b[i]=sum;
	}
	
	for(i = n_ - 1; i >= 0; i--) {
		sum = b[i];
		for(j = i + 1; j < n_; j++)
			sum -= matrix_[i][j] * b[j];
		
		b[i] = sum / matrix_[i][i];
	}
	
	return true;
}

bool MatrixX::Invert() {
	MatrixX tmp(*this);
	int i, j, d;
	
	if(n_ != m_)
		return false;
	
	std::vector<int> indx(n_);
	std::vector<float> col(n_, 0.0);
	
	if(!tmp.ludcmp(indx, &d))
		return false;
	
	for(j = 0; j < n_; j++) {
		for(i = 0; i < n_; i++)
			col[i] = 0.0;
		
		col[j] = 1.0;
		if(!tmp.lubksb(indx,col))
			return false;
		
		for(i = 0; i < n_; i++)
			matrix_[i][j] = col[i];
	}
	
	return true;
}

void MatrixX::Transpose() {
	MatrixX M(m_, n_);
	unsigned int i, j;
	
	for (i = 0; i < m_; i++) {
		for (j = 0; j < n_; j++) {
			if (i != j) {
				M[i][j] = matrix_[j][i];
			}
		}
	}
	
	Copy(M);
}

// this = A * B
bool MatrixX::Mult(const MatrixX& A, const MatrixX& B) {
	unsigned int i, j, k;
	
	if(A.m_ != B.n_)
		return false;
	
	destroy();
	
	create(A.n_, B.m_);
	
	for (i = 0; i < A.n_; i++) {
		for (j = 0; j < B.m_; j++) {
			float subt = 0;
			for (k = 0; k < A.m_; k++)
				subt += A.matrix_[i][k] * B.matrix_[k][j];
			matrix_[i][j] = subt;
		}
	}
	
	return true;
}

MatrixX::~MatrixX() {
	destroy();
}
