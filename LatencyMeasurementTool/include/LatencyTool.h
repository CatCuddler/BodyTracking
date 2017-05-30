#pragma once

#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

class LatencyTool {

private:
	int fps;
	float width;
	float height;
	
	float scale;
	
	Mat posDataObj0;
	Mat posDataObj1;
	
	void plotLine(Mat& image, const Mat& mat, Scalar color);
	void plotCircle(Mat& image, const vector<Point2f>& vector);
	
	Mat normaliseMat(const Mat& mat);
	
	void savePositionData(Mat pos0, Mat pos1);
	void savePeaks(vector<Point2f> peaks);
	
public:
	LatencyTool(int fps, float width, float height);
	void updatePositions(Point2f point0, Point2f point1);
	
	vector<Point2f> findPositionPeaks(Mat posDataObj);
	
	// Plot functions
	void plotPositionsGraph();
	
	float countFrames();
	
};
