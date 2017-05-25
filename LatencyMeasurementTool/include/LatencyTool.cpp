#include "LatencyTool.h"

#include <opencv2/plot.hpp>

#include <iostream>

#include "Peak.h"

LatencyTool::LatencyTool(int fps, float width, float height) : fps(fps), width(width), height(height) {
	
}

void LatencyTool::updatePositions(Point2f point0, Point2f point1) {

	Mat row0 = (Mat_<float>(1,2) << point0.x, point0.y);
	posDataObj0.push_back(row0);
	
	Mat row1 = (Mat_<float>(1,2) << point1.x, point1.y);
	posDataObj1.push_back(row1);
	
}

vector<Point2f> LatencyTool::findPositionPeaks(Mat posDataObj) {
	Mat data;
	for (int i = 0; i < posDataObj.rows; ++i) {
		Mat row = (Mat_<float>(1,1,CV_32F) << posDataObj.at<float>(i, 0));
		data.push_back(row);
		//cout << " " << row << endl;
	}
	
	Peak* peak = new Peak();
	Mat minMax;
	peak->findPeaks(data, minMax);
	
	vector<Point2f> peaks;
	cout << "Peak count: " << minMax.rows << endl;
	for (int i = 0; i < minMax.rows; ++i) {
		Point2i pos(minMax.at<int>(i, 0), minMax.at<int>(i, 1));
		Point2f peak(pos.y, posDataObj.at<float>(pos.y, 0));
		//cout << "Peak pos" << minMax.row(i) << " Peak " << peak << endl;
		peaks.push_back(peak);
	}
	return peaks;
}


void LatencyTool::plotPositionsGraph() {
	// Create black empty images
	Mat image(height, width, CV_8UC3, Scalar(255,255,255));
	
	// Draw grid
	int dist = 100;
	
	int width = image.size().width;
	int height = image.size().height;
	
	for(int i = 0; i < height; i+=dist)
		line(image, Point(0,i), Point(width,i), Scalar(0,0,0), 1, 1);
	
	for(int i = 0; i < width; i+=dist)
		line(image, Point(i,0), Point(i,height), Scalar(0,0,0), 1, 1);
	
	
	// Plot positions for the first object
	plotLine(image, posDataObj0, Scalar(0,0,255));
	
	// Get peaks for the first object
	vector<Point2f> peaks = findPositionPeaks(posDataObj0);
	plotCircle(image, peaks);
	
	
	// Plot positions for the second object
	plotLine(image, posDataObj1, Scalar(0,255,0));
	
	// Get peaks for the second object
	peaks = findPositionPeaks(posDataObj1);
	plotCircle(image, peaks);
	
	
	imshow("plot0", image);
}

void LatencyTool::plotLine(Mat& image, const Mat& mat, Scalar color) {
	for (int i = 1; i < mat.rows; ++i) {
		Point2d lastPosition(i-1, mat.at<float>(i-1, 0));
		Point2d currentPosition(i, mat.at<float>(i, 0));
		
		//cout << "Last Pos: " << lastPosition << " Current Pos: " << currentPosition << endl;
		line(image, lastPosition, currentPosition, color, 1, 1);
	}
}

void LatencyTool::plotCircle(Mat& image, const vector<Point2f>& vector) {
	for (int i = 0; i < vector.size(); ++i) {
		Point2f peak = vector.at(i);
		//cout << " Peak " << peak << endl;
		circle(image, peak, 2, Scalar(255,0,0), 2, 8);
	}
}

float LatencyTool::countFrames() {
	vector<Point2f> peaks0 = findPositionPeaks(posDataObj0);
	vector<Point2f> peaks1 = findPositionPeaks(posDataObj1);
	
	CV_Assert(peaks0.size() == peaks1.size());
	
	float avrFrames = 0;
	
	for (int i = 0; i < peaks0.size(); ++i) {
		Point2f peak = peaks0.at(i);
		int frame0 = peak.x;
		
		peak = peaks1.at(i);
		int frame1 = peak.x;
		
		//cout << "frame0: " << frame0 << " frame1: " << frame1 << endl;
		
		int diff = frame1 - frame0;
		avrFrames += diff;
	}
	
	avrFrames = avrFrames/peaks0.size();
	
	return avrFrames;
}
