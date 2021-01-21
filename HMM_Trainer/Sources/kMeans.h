#pragma once

#include <string>
#include <vector>

extern const std::string trackerNames[6];

void setTrainingFilePath(std::string trainingFilePath);
void setTrainingFileName(std::string trainingFileName);
void setWriteFilePath(std::string writeFilePath);
void setWriteFileName(std::string writeFileName);
void setValidationFilePath(std::string ValidationFilePath);
void setValidationFileName(std::string ValidationFileName);

class Point {
private:
	int idPoint, idCluster;
	std::vector<double> values;
	int totalValue = 0;
public:
	Point();
	Point(int idPoint);
	Point(int idPoint, std::vector<double>& values);
	int getID();
	void setCluster(int idCluster);
	int getCluster();
	double getValue(int index);
	int getTotalValues();
	void addValue(double value);
};

class Cluster {
private:
	int idCluster;
	std::vector<double> centralValues;
	std::vector<Point> points;
public:
	Cluster(int idCluster, Point point);
	void addPoint(Point point);
	bool removePoint(int idPoint);
	double getCentralValue(int index);
	void setCentralValue(int index, double value);
	Point getPoint(int index);
	int getTotalPoints();
	int getID();
};

class KMeans {
private:
	int emissions; // Number of clusters to be created
	int totalValues; // Dimension of the input points (x,y,z,rotx,roty,rotz,royw)
	int maxIterations, totalPoints;
	int averagePoints; // Average number of points per sequence
	std::vector <Cluster> clusters; // vector of clusters
	
	// Returns the id of the closest ClusterCenter of a given point, using the euclidean distance
	int getIDClosestCenter(Point point);
public:
	KMeans();
	KMeans(std::string filePath, std::string fileName);
	KMeans(int K, int totalValues, int totalPoints, int averagePoints, int maxIterations);
	void runKMeans(std::vector<Point> & points);
	std::vector<int> matchPointsToClusters(std::vector<Point> points);
	void writeKMeans(std::string filePath, std::string fileName);
	std::vector<Cluster> getClusters();
	int getAveragePoints();
	double getFinalDistance(std::vector<Point> & points);
};

std::vector<std::vector<Point>> readData(std::string fileName, int fileAmount);
std::vector<KMeans> calculateClusters(int startFile, int fileAmount, int emissions, int totalValues, int maxIterations);
std::vector<std::vector<std::vector<int>>> sortDataToClusters(std::string fileNameToBeSorted, int fileAmount, std::vector<KMeans> kmeans);
std::vector<Point> normaliseMeasurements(std::vector<Point>, int dataVolume);
