/********************************************************************************
 * file name: kMeans.cpp
 * authors: Markus Stabel, Moritz Kolvenbach, Marco Fendrich
 * last changes: 15.03.2018
 * content: means to calculate clusters of data points in varying forms. Used to
 create HMMs and calculate probabilities of new data sets
 ********************************************************************************/

#include "kMeans.h"
#include <vector>
#include <string>
#include <math.h>
#include <array>
#include <iostream>
#include <algorithm>
#include <stdlib.h>
#include <fstream>
#include <time.h>

using namespace std;

// publicly available tracker names to represent them as string and not only as number. Mainly used for console output 
const string trackerNames[6] = { "Head Mounted Display", "Left Hand Controller", "Right Hand Controller", "Back Tracker", "Left Foot Tracker", "Right Foot Tracker" };

string trainingFilePathKMeans;
string trainingFileNameKMeans;
string writeFilePathKMeans;
string writeFileNameKMeans;

void setTrainingFilePath(string trainingFilePath) { trainingFilePathKMeans = trainingFilePath; }
void setTrainingFileName(string trainingFileName) { trainingFileNameKMeans = trainingFileName; }
void setWriteFilePath(string writeFilePath) { writeFilePathKMeans = writeFilePath; }
void setWriteFileName(string writeFileName) { writeFileNameKMeans = writeFileName; }

/********************************************************************************
 * method:		calculateClusters
 * description:	calculates clusters coordinates for multiple data points,
 trackers and files
 * parameters:	startfile is the number of the file to start the process with
 fileAmount is the number of files to be used for the calculation
 emissions is the number of clusters to be created based on the data points
 total_values is the dimension of the points used as input
 maxIteration is the maximum numbers of iterations of the algorithm
 * return value: vector (trackers) of calculated clusters of given points
 ********************************************************************************/
vector<KMeans> calculateClusters(int startFile, int fileAmount, int emissions, int totalValues, int maxIterations) {
	vector<KMeans> returnVector(6);
	vector<vector<Point>> parsedPoints = readData(trainingFileNameKMeans, fileAmount);
	for (int ii = 0; ii < 6; ii++) {
		if (!parsedPoints.at(ii).empty()) {
			KMeans kmeans(emissions, totalValues, (int)parsedPoints.at(ii).size(), (int)parsedPoints.at(ii).size() / fileAmount, maxIterations);
			cout << "Calculating clusters for " << trackerNames[ii] << "; ";
			kmeans.runKMeans(parsedPoints.at(ii));
			kmeans.writeKMeans(writeFilePathKMeans, writeFileNameKMeans + "_" + to_string(ii));
			returnVector.at(ii) = kmeans;
		}
	}
	return returnVector;
}

/********************************************************************************
 * method:		sortDataToClusters
 * description:	sorts the given data set to the given clusters
 * parameters:	fileName is the base name of the files to be read
 fileAmount is the number of files to be used for the calculation
 kmeans are the clusters for each tracker
 * return value: vector (trackers) of vector (files) of vector (data points) of
 cluster numbers
 ********************************************************************************/
vector<vector<vector<int>>> sortDataToClusters(string fileName, int fileAmount, vector<KMeans> kmeans) {
	vector<vector<vector<int>>> returnVector(6);
//    cout << "Normalising data set to same total movement duration. \n";
	vector<vector<Point>> currentDataSet;
	for (int currentFile = 0; currentFile < fileAmount; currentFile++) {
		// check whether there is more than one file to be checked, and creeate seperate files if it is the case
		if (fileAmount != 1) { currentDataSet = readData(fileName + to_string(currentFile), 1); }
		// else just create one file
		else { currentDataSet = readData(fileName, 1); }
		for (int currentTracker = 0; currentTracker < 6; currentTracker++) {
			// if the vector at the currentTracker position is not empty
			if (!currentDataSet.at(currentTracker).empty()) {
				// normalises a given data set of one tracker, matches it to the clusters of the given kMeans and adds it to the returnVector at the tracker's postition
				returnVector.at(currentTracker).push_back(kmeans.at(currentTracker).matchPointsToClusters(normaliseMeasurements(currentDataSet.at(currentTracker), kmeans.at(currentTracker).getAveragePoints())));
			}
		}
	}
	return returnVector;
}


/********************************************************************************
 * method:		normaliseMeasurements
 * description:	normalises a given dataSet
 * parameters:	inputData is the dataSet to be normalised
 dataVolume is the targetSize of the dataSet after normalisation
 * return value: vector<Point> the normalised dataSet
 ********************************************************************************/
vector<Point> normaliseMeasurements(vector<Point> inputData, int dataVolume) {
	vector<Point> returnVector(dataVolume);
	dataVolume--;
	// relative increment size between dataSet and target dataVolume
	double increment = (inputData.size() - 1.0) / dataVolume;
	double currentPos;
	returnVector.at(0) = inputData.at(0);
	for (int ii = 1; ii < dataVolume; ii++) {
		currentPos = increment * ii;
		Point point = Point(ii);
		for (int jj = 0; jj < inputData.at(0).getTotalValues(); jj++) {
			// recalculates each point of inputData in dependance from the increment calculated at the top
			point.addValue((inputData.at(ceil(currentPos)).getValue(jj) - inputData.at(floor(currentPos)).getValue(jj)) * (currentPos - (floor(currentPos))) + inputData.at(floor(currentPos)).getValue(jj));
		}
		returnVector.at(ii) = point;
	}
	returnVector.at(dataVolume) = inputData.at(inputData.size() - 1);
	return returnVector;
}

/********************************************************************************
 * method:		readData
 * description:	reads "fileAmount" number of files into a datatype with wich the
 HMM_Trainer can work
 * parameters:	filename is the filepath for the files to be parsed
 fileAmount is the amount of files to be parsed starting with the
 first
 * return value: vector of (trackers) of vector of <Point>
 ********************************************************************************/
vector<vector<Point>> readData(string fileName, int fileAmount) {
	fstream f;
	vector<vector<Point>> returnVector(6);
	
	string tag, time;
	double posX, posY, posZ;
	double rotX, rotY, rotZ, rotW;
	
	for (int kk = 0; kk < 0 + fileAmount; kk++) {
		
		string fullPath;
		// if the amount of files is > 1 create a different file for each file
		if (fileAmount != 1) fullPath = (trainingFilePathKMeans + fileName + to_string(kk) + ".csv");
		// else only create one path
		else fullPath = (trainingFilePathKMeans + fileName + ".csv");
		
		if (ifstream(fullPath)) {
			f.open(fullPath);
		} else {
			cout << "The current data set file " << fullPath << " does not exist!\n\n\n";
			throw invalid_argument("File does not exist");
		}
		
		f >> tag >> time >> tag >> tag >> tag >> tag >> tag >> tag >> tag; // Skip header
		
		int ii = 0;
		for (;;) {
			f >> tag >> time >> posX >> posY >> posZ >> rotX >> rotY >> rotZ >> rotW;
			vector<double> values = { posX, posY, posZ, rotX, rotY, rotZ, rotW };
			Point point = Point(ii, values);
			
			// differentiate the parsed points and add them to the correct vectors
			if (tag.compare("head") == 0)      returnVector.at(0).push_back(point);
			else if (tag.compare("lHand") == 0) returnVector.at(1).push_back(point);
			else if (tag.compare("rHand") == 0) returnVector.at(2).push_back(point);
			else if (tag.compare("hip") == 0) returnVector.at(3).push_back(point);
			else if (tag.compare("lFoot") == 0) returnVector.at(4).push_back(point);
			else if (tag.compare("rFoot") == 0) returnVector.at(5).push_back(point);
			else cout << "Error! Unknown tracker data detected.";
			
			++ii;
			
			if (f.fail() || f.eof())
				break;
		}
		f.close();
	}
	return returnVector;
}


/********************************************************************************
 * method:		KMeans constructor
 * description:	creates a KMeans object with the given input values
 * parameters:	emissions is the amount of clusters to be created
 totalValue the dimension of the given points to be clustered
 totalPoints the amount of points in the dataSet
 averagePoints the average amount of points belonging to each cluster
 maxIterations the max amount of iterations for the given KMeans
 ********************************************************************************/
KMeans::KMeans(int emissions, int totalValues, int totalPoints, int averagePoints, int maxIterations) {
	this->emissions = emissions;
	this->totalValues = totalValues;
	this->totalPoints = totalPoints;
	this->averagePoints = averagePoints;
	this->maxIterations = maxIterations;
}

/********************************************************************************
 * method:		KMeans constructor
 * description:	creates a KMeans object from a file
 * parameters:	filePath the filepath to the KMeans file
 fileName the name of the KMeans file
 ********************************************************************************/
KMeans::KMeans(string filePath, string fileName) {
	fstream f;
	string fullPath = filePath + fileName + "_cluster.txt";
	if (ifstream(fullPath)) {
		f.open(fullPath);
	} else {
		throw invalid_argument("No corresponding file found!");
	}
	
	f >> emissions >> totalValues >> maxIterations >> totalPoints >> averagePoints;
	
	double x, y, z;
	
	for (int ii = 0; ii < emissions; ii++) {
		f >> x >> y >> z;
		vector<double> values = { x, y, z };
		Point point = Point(ii, values);
		Cluster cluster(ii, point);
		clusters.push_back(cluster);
	}
	f.close();
}

/********************************************************************************
 * method:		KMeans null constructor
 * description:	creates an empty KMeans object
 ********************************************************************************/
KMeans::KMeans() : emissions(0), totalValues(0), totalPoints(0), averagePoints(0), maxIterations(0) {}

vector<Cluster> KMeans::getClusters() { return clusters; }
int KMeans::getAveragePoints() { return averagePoints; }

/********************************************************************************
 * method:		getIDClosestCenter
 * description:	calculates the closest cluster for a given point and returns it's ID
 * parameters:	point the point for which the closest cluster is to be found
 * return value: int the ID of the closest cluster from the point
 ********************************************************************************/
int KMeans::getIDClosestCenter(Point point) {
	double sum = 0.0, minDistance;
	int idClusterCenter = 0;
	
	// check euclidean distance to the first cluster
	for (int i = 0; i < totalValues; i++) {
		sum += pow(clusters[0].getCentralValue(i) - point.getValue(i), 2.0);
	}
	minDistance = sqrt(sum);
	
	// check distance for all the other cluster central_values
	for (int ii = 1; ii < emissions; ii++) {
		double distance;
		sum = 0.0;
		for (int jj = 0; jj < totalValues; jj++) {
			sum += pow(clusters[ii].getCentralValue(jj) - point.getValue(jj), 2.0);
		}
		distance = sqrt(sum);
		
		// update the minimal distance
		if (distance < minDistance) {
			minDistance = distance;
			idClusterCenter = ii;
		}
	}
	return idClusterCenter;
}

/********************************************************************************
 * method:		runKMeans
 * description:	uses KMeans clustering algorithm to cluster the given set of points
 * parameters:	points a vector<Point>
 ********************************************************************************/
void KMeans::runKMeans(vector<Point> & points) {
	if (emissions > totalPoints) { return; }
	
	vector<int> blockedIndexes;
	
	// pick <emissions> initial cluster_central_values
	for (int ii = 0; ii < emissions; ii++) {
		while (true) {
			int point_index = rand() % totalPoints;
			
			if (find(blockedIndexes.begin(), blockedIndexes.end(), point_index) == blockedIndexes.end()) {
				blockedIndexes.push_back(point_index);
				points[point_index].setCluster(ii);
				Cluster cluster(ii, points[point_index]);
				clusters.push_back(cluster);
				break;
			}
		}
	}
	int iter = 1;
	while (true) {
		// a boolean to check the exit condition whether the clusters do still get updated
		bool done = true;
		
		// assoiciate each point to its closest cluster
		for (int ii = 0; ii < totalPoints; ii++) {
			int idOldCluster = points[ii].getCluster();
			
			// calculate closest cluster
			int idClosestCluster = getIDClosestCenter(points[ii]);
			
			// check whether closest_cluster = old_cluster and if not update old_cluster
			if (idOldCluster != idClosestCluster) {
				// check if the point was already matched to a cluster, if so remove it from the old cluster
				if (idOldCluster != -1) {
					clusters[idOldCluster].removePoint(points[ii].getID());
				}
				
				points[ii].setCluster(idClosestCluster);
				clusters[idClosestCluster].addPoint(points[ii]);
				done = false;
			}
		}
		
		// recalculating central values of the cluster
		
		// iterateor for the clusters
		for (int ii = 0; ii < emissions; ii++) {
			// iterator for the dimension
			for (int jj = 0; jj < totalValues; jj++) {
				int totalPointsCluster = clusters[ii].getTotalPoints();
				double sum = 0.0;
				
				if (totalPointsCluster > 0) {
					// iterator for the points of each cluster
					for (int kk = 0; kk < totalPointsCluster; kk++)
						sum += clusters[ii].getPoint(kk).getValue(jj);
					clusters[ii].setCentralValue(jj, sum / totalPointsCluster);
				}
			}
		}
		
		if (done == true) {
			cout << "finished in iteration " << iter << ".\n";
			break;
		}
		if (iter >= maxIterations) {
			cout << " reached maximum number of iterations (" << maxIterations << "). Using preliminary clusters.\n";
			break;
		}
		iter++;
	}
}

/********************************************************************************
 * method:		matchPointsToClusters
 * description:	matches each element of a given vector<Point> to to its cluster
 * parameters:	points a vector<Point>
 * return value: vector<int> containing the ids of the closest cluster in the
 order as  the input vector
 ********************************************************************************/
vector<int> KMeans::matchPointsToClusters(vector<Point> points) {
	vector<int> returnVector(points.size(), 0);
	// find closest cluster ID for each element in points
	for (int ii = 0; ii < points.size(); ii++) {
		returnVector.at(ii) = getIDClosestCenter(points[ii]);
	}
	return returnVector;
}

/********************************************************************************
 * method:		writeKMeans
 * description:	writes a KMeans object into a .txt file nameFormat : <fileName>_cluster.txt
 * parameters:	filePath the filepath where the .txt file is to be saved
 fileName the name of the file to be created
 ********************************************************************************/
void KMeans::writeKMeans(string filePath, string fileName) {
	ofstream file;
	file.open(filePath + fileName + "_cluster.txt", ios::out | ios::trunc);
	if (file.is_open()) {
		
		file << emissions << " " << totalValues << " " << maxIterations << " " << totalPoints << " " << averagePoints << endl;
		
		for (int ii = 0; ii < getClusters().size(); ii++) {
			for (int jj = 0; jj < 7; jj++) {
				file << getClusters()[ii].getCentralValue(jj) << " ";
			}
			file << endl;
		}
		file.close();
	} else {
		cout << "Unable to write calculated clusters into file.\n";
	}
}


/********************************************************************************
 * method:		Cluster constructor
 * description:	creates a Cluster object with the given input
 * parameters:	idCluster the ID for the cluster to be constructed
 point the coordinates of the cluster
 ********************************************************************************/
Cluster::Cluster(int idCluster, Point point) {
	this->idCluster = idCluster;
	int totalValues = point.getTotalValues();
	
	for (int i = 0; i < totalValues; i++)
		centralValues.push_back(point.getValue(i));
	
	points.push_back(point);
}

void Cluster::addPoint(Point point) {
	points.push_back(point);
}

bool Cluster::removePoint(int idPoint) {
	int totalPoints = (int)points.size();
	
	for (int i = 0; i < totalPoints; i++) {
		if (points[i].getID() == idPoint) {
			points.erase(points.begin() + i);
			return true;
		}
	}
	return false;
}

void Cluster::setCentralValue(int index, double value) { centralValues[index] = value; }
double Cluster::getCentralValue(int index) { return centralValues[index]; }
Point Cluster::getPoint(int index) { return points[index]; }
int Cluster::getTotalPoints() { return (int)points.size(); }
int Cluster::getID() { return idCluster; }


/********************************************************************************
 * method:		Point constructor
 * description:	creates a Point object with the given input
 * parameters:	idPoint the id for the point
 values the coordinates of the point
 ********************************************************************************/
Point::Point(int idPoint, vector<double>& values) {
	this->idPoint = idPoint;
	totalValue = (int)values.size();
	
	for (int i = 0; i < totalValue; i++)
		this->values.push_back(values[i]);
	
	idCluster = -1;
}

Point::Point(int idPoint) : idPoint(idPoint), idCluster(-1) {}

Point::Point() : idPoint(-1), idCluster(-1) {}

void Point::setCluster(int idCluster) { this->idCluster = idCluster; }
int Point::getCluster() { return idCluster; }
int Point::getID() { return idPoint; }
double Point::getValue(int index) { return values[index]; }
int Point::getTotalValues() { return totalValue; }

void Point::addValue(double value) {
	values.push_back(value);
	totalValue++;
}
