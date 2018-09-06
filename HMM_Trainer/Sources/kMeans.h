#ifndef KMEANS_H_INCLUDED
#define KMEANS_H_INCLUDED

//#include <vector>

using namespace std;

extern const string trackerNames[6];

void setTrainingFilePath(string trainingFilePath);
void setTrainingFileName(string trainingFileName);
void setWriteFilePath(string writeFilePath);
void setWriteFileName(string writeFileName);

class Point {
private:
	int idPoint, idCluster;
	vector<double> values;
	int totalValue = 0;
public:
	Point();
	Point(int idPoint);
	Point(int idPoint, vector<double>& values);
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
	vector<double> centralValues;
	vector<Point> points;
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
	int emissions; // number of clusters to be created
	int totalValues; // dimension of the input points (x,y,z)
	int maxIterations, totalPoints;
	int averagePoints; // average number of points per sequence
	vector <Cluster> clusters; // vector of clusters

	// returns the id of the closest ClusterCenter of a given point, using the euclidean distance
	int getIDClosestCenter(Point point);
public:
	KMeans();
	KMeans(string filePath, string fileName);
	KMeans(int K, int totalValues, int totalPoints, int averagePoints, int maxIterations);
	void runKMeans(vector<Point> & points);
	vector<int> matchPointsToClusters(vector<Point> points);
	void writeKMeans(string filePath, string fileName);
	vector<Cluster> getClusters();
	int getAveragePoints();
};

vector<vector<Point>> readData(string fileName, int fileAmount);
vector<KMeans> calculateClusters(int startFile, int fileAmount, int emissions, int totalValues, int maxIterations);
vector<vector<vector<int>>> sortDataToClusters(string fileNameToBeSorted, int fileAmount, vector<KMeans> kmeans);
vector<Point> normaliseMeasurements(vector<Point>, int dataVolume);

#endif