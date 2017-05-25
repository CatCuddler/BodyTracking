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
	
	void plot(Mat data);

	
public:
	LatencyTool(int fps, float width, float height);
	void updatePositions(Point2f point0, Point2f point1);
	
	vector<Point2i> findPositionPeaks(Mat posDataObj);
	
	// Plot functions
	void plotPositionsGraph();
	void plotVelocityGraph();
	
};
