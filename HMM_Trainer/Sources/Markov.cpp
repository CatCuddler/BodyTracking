/********************************************************************************
* file name: Markov.cpp
* author: Markus Stabel
* last changes: 08.03.2018
* content: contains the function definitions of the HMM class from Markov.h. The 
  naming convention follows "A Tutorial on Hidden Markov Models and Selected
  Applications in Speech Recognition" by Rabiner, 1989.
********************************************************************************/

#pragma once
#include "Markov.h"
#include "matrix.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>
#include <algorithm>    // std::min
#include <cmath>
using std::ofstream;
using std::ifstream;
using std::cout;
using std::getline;
using std::ios;
using std::stringstream;

/********************************************************************************
* method:		HMM construtor
* description:	initializes a HMM with either fully random probabilities or a 
				left-to-right model
* parameters:	N is the number of states
*				M is the number of possible emissions
*				LRdepth is the number of possible transitions in a left-to-right 
				model. Defaults to 0, in which case all probabilities are randomized
* return value: Hidden Markov Model
********************************************************************************/
HMM::HMM(int N, int M, int LRdepth): numStates(N), sigmaSize(M), pi(numStates, 0), probabilityThreshold(0) {
	a = matrix<double>(0, numStates, numStates);
	b = matrix<double>(0, numStates, sigmaSize);
	
	srand(time(NULL)); // Seed for random number generation

	// Initialize Pi
	if (LRdepth == 0) {		// left-to-right depth of 0: random initialization
		double sum = 0;
		for (int i = 0; i < N; i++) {
			int randomInt = rand() % 1000;
			pi.at(i) = randomInt;
			sum += randomInt;
		}

		for (int i = 0; i < N; i++) {
			pi.at(i) /= sum;
		}
	} else {				// left-to-right model
		pi.at(0) = 1;
	}

	// Initialize A
	if (LRdepth == 0) {		// left-to-right depth of 0: random initialization
		for (int i = 0; i < N; i++) {

			double sum = 0;
			for (int j = 0; j < N; j++) {
				int randomInt = rand() % 1000;
				a[i][j] = randomInt;
				sum += randomInt;
			}

			for (int j = 0; j < N; j++) {
				a[i][j] /= sum;
			}

		}
	} else {				// left-to-right model
		for (int i = 0; i < N; i++) {
			for (int j = i; j < std::min(i + 1 + LRdepth, N); j++) {
				a[i][j] = 1.0 / std::min(LRdepth + 1, N - i);
			}
		}
	}

	// Initialize B
	for (int i = 0; i < N; i++) {

		double sum = 0;
		for (int j = 0; j < M; j++) {
			int randomInt = rand() % 1000;
			b[i][j] = randomInt;
			sum += randomInt;
		}

		for (int j = 0; j < M; j++) {
			b[i][j] /= sum;
		}
	}

}

/********************************************************************************
* method:		default HMM construtor
* description:	initializes a HMM
* parameters:	none
* return value:	Hidden Markov Model
********************************************************************************/
HMM::HMM(): numStates(1), sigmaSize(1), pi(numStates, 0), probabilityThreshold(0) {
	a = matrix<double>(0, numStates, numStates);
	b = matrix<double>(0, numStates, sigmaSize);
}

/********************************************************************************
* method:		HMM construtor
* description:	reads a HMM from a file
* parameters:	filePath is the path to the folder in which the HMM is saved
*				fileName is the HMMs name without the _HMM.txt ending
* return value: Hidden Markov Model
********************************************************************************/
HMM::HMM(string filePath, string fileName) {

	string str;

	ifstream file(filePath + fileName + "_HMM.txt");
	if (file.is_open()) {

		// read number of states and number of emissions
		getline(file, str);
		stringstream(str) >> numStates;
		getline(file, str);
		stringstream(str) >> sigmaSize;

		a = matrix<double>(0, numStates, numStates);
		b = matrix<double>(0, numStates, sigmaSize);
		pi = vector<double>(numStates, 0);

		// read A
		for (int i = 0; i < numStates; i++) {
			for (int j = 0; j < numStates; j++) {
				getline(file, str, ';');
				stringstream(str) >> a[i][j];
			}
		}
		// read B
		for (int i = 0; i < numStates; i++) {
			for (int j = 0; j < sigmaSize; j++) {
				getline(file, str, ';');
				stringstream(str) >> b[i][j];
			}
		}

		// read Pi
		for (int i = 0; i < numStates; i++) {
			getline(file, str, ';');
			stringstream(str) >> pi.at(i);
		}

		// read log probability threshold
		getline(file, str, ';');
		stringstream(str) >> probabilityThreshold;

		file.close();

	}
	else {
		cout << "Unable to read file. ";
		throw std::invalid_argument("File does not exist");
	}
}

/********************************************************************************
* method:		getProbabilityThreshold
* description:	returns the HMMs log probability threshold
* parameters:	none
* return value: the log probability threshold
********************************************************************************/
double HMM::getProbabilityThreshold() { return probabilityThreshold; }

/********************************************************************************
* method:		updateAlphaNormalized
* description:	updates the forward probability matrix alpha using normalization 
				and returns the normalization vector
* parameters:	sequence is the observation sequence for which the forward 
				probability is calculated
*				alpha is the forward probability matrix to be updated
* return value: the vector of normalization coefficients
********************************************************************************/
vector<double> HMM::updateAlphaNormalized(vector<int>& sequence, double** alpha) {

	const int N = numStates;
	const int T = sequence.size();

	vector<double> c(T, 0);

	// 1. Initialization
	for (int i = 0; i < N; i++) {
		alpha[i][0] = pi.at(i) * b[i][sequence.at(0)];
		c.at(0) += alpha[i][0];
	}
	c.at(0) = 1 / c.at(0);

	for (int i = 0; i < N; i++) {
		alpha[i][0] *= c.at(0);
	}

	// 2. Induction
	for (int t = 1; t < T; t++) {

		for (int j = 0; j < N; j++) {

			double sum = 0;
			for (int i = 0; i < N; i++) {
				sum += alpha[i][t - 1] * a[i][j];
			}

			alpha[j][t] = sum * b[j][sequence.at(t)];
			c.at(t) += alpha[j][t];
		}

		c.at(t) = 1 / c.at(t);

		for (int j = 0; j < N; j++) {
			alpha[j][t] *= c.at(t);
		}
	}

	return c;
}

/********************************************************************************
* method:		updateBetaNormalized
* description:	updates the backward probability matrix beta using normalization
* parameters:	sequence is the observation sequence for which the backward
				probability is calculated
*				c is the normalization vector taken from updateAlphaNormalized
*				beta is the backward probability matrix to be updated
* return value: the vector of normalization coefficients
********************************************************************************/
void HMM::updateBetaNormalized(vector<int>& sequence, vector<double>& c, double** beta) {

	const int N = numStates;
	const int T = sequence.size();

	// 1. Initialization
	for (int i = 0; i < N; i++) {
		beta[i][T - 1] = c.at(T - 1);
	}

	// 2. Induction
	for (int t = T - 2; t >= 0; t--) {

		for (int i = 0; i < N; i++) {

			double sum = 0;
			for (int j = 0; j < N; j++) {
				sum += a[i][j] * b[j][sequence.at(t + 1)] * beta[j][t + 1];
			}

			beta[i][t] = sum * c.at(t);
		}
	}
}

/********************************************************************************
* method:		updateBetaNormalized
* description:	calculates the log probability of the given sequence
				being emitted by the HMM
* parameters:	sequence is the observation sequence for which the probability is
				calculated
* return value: the log of the sequence's probability being emitted by the  HMM
********************************************************************************/
double HMM::calculateProbability(vector<int>& sequence) {

	const int N = numStates;
	const int T = sequence.size();

	double** alpha = matrix<double>(0, N, T); // forward probability matrix

	vector<double> c = updateAlphaNormalized(sequence, alpha); // run the forward algorithm

	// Calculate probability
	double probability = 0;
	for (int t = 0; t < sequence.size(); t++) {
		probability -= std::log(c.at(t));
	}

	return probability;
}

/********************************************************************************
* method:		trainHMM
* description:	uses the Baum-Welch algorithm to train the HMM for a set of
  observation	sequences
* parameters:	sequence is a vector of observation sequences that are used to 
				train the HMM
*				maxIter is the maximum number of iterations. Defaults to 100
*				delta is the log probability change per iteration at which the 
				algorithm will stop. Defaults to 0.1
* return value: none
********************************************************************************/
void HMM::trainHMM(vector<vector<int>> &sequence, int maxIter, double delta) {

	const int N = numStates;
	const int M = sigmaSize;
	const int sequenceSize = sequence.size();

	double probability = 0;
	double prevProbability = 5 * delta; // making sure that the algorithm doesn't stop on the first iteration
	int iter; // iterator

	// Initialize a forward and a backward matrix per observation sequence
	vector<double **> alpha(sequenceSize, 0);
	vector<double **> beta(sequenceSize, 0);
	vector<vector<double>> c(sequenceSize); // One normalization vector per sequence

	// set the size of each matrix
	for (iter = 0; iter < sequenceSize; iter++) {
		int T = sequence.at(iter).size();
		alpha.at(iter) = matrix<double>(0, N, T);
		beta.at(iter) = matrix<double>(0, N, T);
	}

	// Baum-Welch-Agorithm
	for (iter = 0; (iter < maxIter && abs(probability - prevProbability) > delta); iter++) { // iterate until the probability no longer changes more than delta or for a maximum of maxIter iterations

		// Update forward and backward matrix
		for (int l = 0; l < sequenceSize; l++) {
			c.at(l) = updateAlphaNormalized(sequence.at(l), alpha.at(l));
			updateBetaNormalized(sequence.at(l), c.at(l), beta.at(l));
		}

		// Update Pi
		for (int i = 0; i < N; i++) {

			double nominator = 0;
			double denominator = 0;

			for (int l = 0; l < sequenceSize; l++) {
				nominator += alpha.at(l)[i][0] * beta.at(l)[i][0];
				for (int j = 0; j < N; j++) {
					denominator += alpha.at(l)[j][0] * beta.at(l)[j][0];
				}
			}

			pi.at(i) = nominator / denominator;

		}

		// Update A
		for (int i = 0; i < N; i++) {
			for (int j = 0; j < N; j++) {

				double nominator = 0;
				double denominator = 0;

				for (int l = 0; l < sequenceSize; l++) {
					for (int t = 0; t < sequence.at(l).size() - 1; t++) {
						nominator += alpha.at(l)[i][t] * a[i][j] * b[j][sequence.at(l).at(t + 1)] * beta.at(l)[j][t + 1];
						denominator += alpha.at(l)[i][t] * beta.at(l)[i][t] / c.at(l).at(t);
					}
				}

				a[i][j] = nominator / denominator;

			}
		}

		// Update B
		for (int j = 0; j < N; j++) {
			for (int k = 0; k < M; k++) {

				double nominator = 0;
				double denominator = 0;

				for (int l = 0; l < sequenceSize; l++) {
					for (int t = 0; t < sequence.at(l).size(); t++) {
						if (sequence.at(l).at(t) == k) {
							nominator += alpha.at(l)[j][t] * beta.at(l)[j][t] / c.at(l).at(t);
						}
						denominator += alpha.at(l)[j][t] * beta.at(l)[j][t] / c.at(l).at(t);
					}
				}

				b[j][k] = nominator / denominator;

			}
		}

		// Calculate log probability
		prevProbability = probability;
		probability = 0;
        if (sequence.size()>0){
		for (int t = 0; t < sequence.at(0).size(); t++) {
			probability -= std::log(c.at(0).at(t));
		}
        }
	}

	std::cout << iter;

	// Save log probability threshold
	probabilityThreshold = probability * 2;

}

/********************************************************************************
* method:		writeHMM
* description:	writes the HMM to a file
* parameters:	filePath is the path to the folder in which the HMM is saved
*				fileName is the HMMs name without the _HMM.txt ending
* return value: none
********************************************************************************/
void HMM::writeHMM(string filePath, string fileName) {

	ofstream file;
	file.open(filePath + fileName + "_HMM.txt", ios::out | ios::trunc);
	if (file.is_open()) {
		// write number of states and number of emissions
		file << numStates << ";\n";
		file << sigmaSize << ";\n";

		// write A
		for (int i = 0; i < numStates; i++) {
			for (int j = 0; j < numStates; j++) {
				file << a[i][j] << ";";
			}
			file << "\n";
		}

		// write B
		for (int i = 0; i < numStates; i++) {
			for (int j = 0; j < sigmaSize; j++) {
				file << b[i][j] << ";";
			}
			file << "\n";
		}

		// write Pi
		for (int i = 0; i < numStates; i++) {
			file << pi.at(i) << ";";
		}
		file << "\n";

		// write threshold probability
		file << probabilityThreshold << ";";

		file.close();
	}
	else {
		cout << "Unable to write HMM.\n";
	}
}
