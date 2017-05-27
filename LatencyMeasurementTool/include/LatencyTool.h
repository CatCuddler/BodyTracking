#pragma once

#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

class LatencyTool {

private:
	int fps;
	float width;
	float height;
	
	Mat posDataObj0;
	Mat posDataObj1;
	
	void plotLine(Mat& image, const Mat& mat, Scalar color);
	void plotCircle(Mat& image, const vector<Point2f>& vector);
	
	void savePositionData();
	
public:
	LatencyTool(int fps, float width, float height);
	void updatePositions(Point2f point0, Point2f point1);
	
	vector<Point2f> findPositionPeaks(Mat posDataObj);
	
	// Plot functions
	void plotPositionsGraph();
	
	float countFrames();
	
};
