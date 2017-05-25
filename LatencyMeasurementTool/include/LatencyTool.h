#pragma once

#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

class LatencyTool {

private:
	
	int fps;
	
	Mat posDataObj0;
	Mat posDataObj1;
	
	void plot(Mat data);

	
public:
	LatencyTool(int fps);
	void updatePositions(Point2f point0, Point2f point1);
	
	// Plot functions
	void plotPositionsGraph();
	void plotVelocityGraph();
	
};
