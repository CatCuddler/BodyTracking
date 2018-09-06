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

#ifndef MARKOV_H_INCLUDED
#define MARKOV_H_INCLUDED

#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

using std::string;
using std::vector;

/***********************************************************
* class: HMM
* description: an object of the class HMM contains all
  parameters of a Hidden Markov Model.
***********************************************************/
class HMM {

	// TODO public for now as throwing errors in BodyTracking otherwise
public:
	int numStates; // number of possible states
	int sigmaSize; // size of the emmission alphabet
	double **a; // transition matrix
	double **b; // emmission matrix
	vector<double> pi; // initial state distribution
	double probabilityThreshold; // log probability threshold for gesture to be recognized

	vector<double> updateAlphaNormalized(vector<int>& sequence, double** alpha);
	void updateBetaNormalized(vector<int>& sequence, vector<double>& c, double** beta);

public:
	HMM(int N, int M, int LRdepth = 0);
	HMM();
	HMM(string, string);

	void writeHMM(string filePath, string fileName);
	double getProbabilityThreshold();
	double calculateProbability(vector<int>& sequence);
	void trainHMM(vector<vector<int>>& sequence, int maxIter = 100, double delta = 0.1);

};

#endif