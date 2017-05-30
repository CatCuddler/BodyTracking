#include "LatencyTool.h"

#include <opencv2/plot.hpp>

#include <iostream>

#include "Peak.h"

LatencyTool::LatencyTool(int fps, float width, float height) : fps(fps), width(width), height(height), scale(100) {
	
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
	Mat image(scale, width, CV_8UC3, Scalar(255,255,255));
	
	// Draw grid
	int dist = 50;
	int smallDist = 10;
	
	int width = image.size().width;
	int height = image.size().height;
	
	for(int i = 0; i < height; i += dist) {
		for (int j = 0; j < width; j += smallDist) {
			line(image, Point(j,i), Point(j + smallDist,i), Scalar(0,0,0), 1, 1);
			j += smallDist;
		}
  
	}
	
	for(int i = 0; i < width; i += dist) {
		for (int j = 0; j < height; j += smallDist) {
			line(image, Point(i,j), Point(i,j + smallDist), Scalar(0,0,0), 1, 1);
			j += smallDist;
		}
	}
	
	// Plot normalised positions for the first object
	Mat normPos0 = smoothAndNormaliseMat(posDataObj0);
	plotLine(image, normPos0, Scalar(0,0,255));
	
	// Get peaks for the first object
	vector<Point2f> allPeaks;
	vector<Point2f> peaks = findPositionPeaks(normPos0);
	allPeaks.insert(allPeaks.end(), peaks.begin(), peaks.end());
	plotCircle(image, peaks);
	
	// Plot positions for the second object
	Mat normPos1 = smoothAndNormaliseMat(posDataObj1);
	plotLine(image, normPos1, Scalar(0,255,0));
	
	// Get peaks for the second object
	peaks = findPositionPeaks(normPos1);
	allPeaks.insert(allPeaks.end(), peaks.begin(), peaks.end());
	plotCircle(image, peaks);
	
	savePositionData(normPos0, normPos1);
	savePeaks(allPeaks);
	imwrite("results/positionPlot.png", image);
	imshow("plot0", image);
}

void LatencyTool::plotLine(Mat& image, const Mat& mat, Scalar color) {
	for (int i = 1; i < mat.rows; ++i) {
		Point2d lastPosition(i-1, mat.at<float>(i-1, 0) * scale);
		Point2d currentPosition(i, mat.at<float>(i, 0) * scale);
		
		//cout << "Last Pos: " << lastPosition << " Current Pos: " << currentPosition << endl;
		line(image, lastPosition, currentPosition, color, 1, 1);
	}
}

void LatencyTool::plotCircle(Mat& image, const vector<Point2f>& vector) {
	for (int i = 0; i < vector.size(); ++i) {
		Point2f peak = vector.at(i);
		peak.y = peak.y * scale;
		//cout << " Peak " << peak << endl;
		circle(image, peak, 2, Scalar(255,0,0), 2, 8);
	}
}

float LatencyTool::countFrames() {
	Mat normPos0 = smoothAndNormaliseMat(posDataObj0);
	vector<Point2f> peaks0 = findPositionPeaks(normPos0);
	Mat normPos1 = smoothAndNormaliseMat(posDataObj1);
	vector<Point2f> peaks1 = findPositionPeaks(normPos1);
	
	if (peaks0.size() != peaks1.size()) {
		cout << "Assertion failed: peaks0.size() != peaks1.size()" << endl;
		return -1;
	}
	
	float avrFrames = 0;
	for (int i = 0; i < peaks0.size(); ++i) {
		// Extract the frame number of the first object
		Point2f peak = peaks0.at(i);
		int frame0 = peak.x;
		
		// Extract the frame number of the second object
		peak = peaks1.at(i);
		int frame1 = peak.x;
		
		//cout << "frame0: " << frame0 << " frame1: " << frame1 << endl;
		
		// Calculate the difference
		int diff = frame1 - frame0;
		avrFrames += diff;
	}
	
	avrFrames = avrFrames/peaks0.size();
	
	return avrFrames;
}

Mat LatencyTool::smoothAndNormaliseMat(const Mat& mat) {
	Mat cMat;
	mat.col(0).copyTo(cMat);		// Copy x Positions
	
	const Size& ksize = Size(9, 9);
	GaussianBlur(cMat, cMat, ksize, 0);

	normalize(cMat, cMat, 0, 1, NORM_MINMAX);
	
	return cMat;
}

void LatencyTool::savePositionData(Mat pos0, Mat pos1) {
	std::fstream outputFile;
	outputFile.open("results/positionData.csv", std::ios::out);
	
	outputFile << "x0, x1" << endl;
	
	for (int i = 0; i < pos0.rows - 1; i++) {
		outputFile << pos0.at<float>(i,1) << ", ";
		outputFile << pos1.at<float>(i,1) << ", ";
		outputFile << endl;
	}
	outputFile.close();
}

void LatencyTool::savePeaks(vector<Point2f> peaks) {
	std::fstream outputFile;
	outputFile.open("results/peaksData.csv", std::ios::out);
	
	outputFile << "x, y" << endl;
	
	for (int i = 0; i < peaks.size(); ++i) {
		Point2f peak = peaks.at(i);
		outputFile << peak.x << ", " << peak.y << endl;
	}
	outputFile.close();
}
