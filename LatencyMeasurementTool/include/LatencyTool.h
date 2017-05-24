#pragma once

#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

class LatencyTool {

private:
	
	int fps;
	
	vector<Point2f> positionsObj0;
	vector<Point2f> positionsObj1;
	
	Mat posDataX;
	Mat posDataY;
	
	void plot(Mat data);

	
public:
	LatencyTool(int fps);
	void updatePositions(vector<Point2f> positions);
	
	// Plot functions
	void plotPositionsGraph();
	void plotVelocityGraph();
	
};
