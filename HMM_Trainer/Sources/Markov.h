/***********************************************************
 * file name: Markov.h
 * author: Markus Stabel
 * last changes: 08.03.2018
 * content: contains the class representation of a Hidden 
 Markov Model, including function representations of the
 Baum-Welch- and forward Algorithm. The naming convention
 follows "A Tutorial on Hidden Markov Models and Selected 
 Applications in Speech Recognition" by Rabiner, 1989.
 ***********************************************************/

#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <fstream>

/***********************************************************
 * class: HMM
 * description: an object of the class HMM contains all
 parameters of a Hidden Markov Model.
 ***********************************************************/
class HMMModel {
	
	// TODO public for now as throwing errors in BodyTracking otherwise
public:
	int numStates; // number of hidden states
	int sigmaSize; // size of the emmission alphabet
	double **a; // transition matrix
	double **b; // emmission matrix
	std::vector<double> pi; // initial state distribution
	double probabilityThreshold; // log probability threshold for gesture to be recognized
	
	std::vector<double> updateAlphaNormalized(std::vector<int>& sequence, double** alpha);
	void updateBetaNormalized(std::vector<int>& sequence, std::vector<double>& c, double** beta);
	
public:
	HMMModel(int N, int M, int LRdepth = 0);
	HMMModel();
	HMMModel(std::string, std::string);
	
	void writeHMM(std::string filePath, std::string fileName);
	double getProbabilityThreshold();
	double calculateProbability(std::vector<int>& sequence);
	void trainHMM(std::vector<std::vector<int>>& sequence, int maxIter = 100, double delta = 0.1);
};
