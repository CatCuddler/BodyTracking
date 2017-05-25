#include "LatencyTool.h"

#include <opencv2/plot.hpp>

#include <iostream>

#include "Peak.h"

LatencyTool::LatencyTool(int fps) : fps(fps) {
	
}

void LatencyTool::updatePositions(Point2f point0, Point2f point1) {

	Mat row0 = (Mat_<double>(1,2) << point0.x, point0.y);
	posDataObj0.push_back(row0);
	
	Mat row1 = (Mat_<double>(1,2) << point1.x, point1.y);
	posDataObj1.push_back(row1);
	
}

vector<Point2i> LatencyTool::findPositionPeaks() {
	
	Mat data;
	for (int i = 0; i < posDataObj0.rows; ++i) {
		Mat row = (Mat_<float>(1,1,CV_32F) << posDataObj0.at<double>(i, 0));
		//Mat row = (Mat_<float>(1,2,CV_32F) << i, posDataObj0.at<double>(i, 0));
		data.push_back(row);
		//cout << " " << row << endl;
	}
	
	//Mat data = (Mat_<float>(1,10,CV_32F) << 0, -1, -20, -1, 0, 1, 2, 30, 2, 0, -1);

	Peak* peak = new Peak();
	Mat maxima;
	peak->findPeaks(data, maxima);
	
	vector<Point2i> peakPos;
	cout << "peak num " << maxima.rows << endl;
	for (int i = 0; i < maxima.rows; ++i) {
		Point2i pos(maxima.at<int>(i, 0), maxima.at<int>(i, 1));
		cout << "peak " << maxima.row(i) << " " << pos.x << " " << pos.y << endl;
		peakPos.push_back(pos);
	}
	return peakPos;

}


void LatencyTool::plotPositionsGraph() {
	
	// Plot positions for the first object
	
	// Create black empty images
	Mat image(500, 500, CV_8UC3, Scalar(255,255,255));
	
	// Draw grid
	int dist = 100;
	
	int width = image.size().width;
	int height = image.size().height;
	
	for(int i = 0; i < height; i+=dist)
		line(image, Point(0,i), Point(width,i), Scalar(0,0,0), 1, 1);
	
	for(int i = 0; i < width; i+=dist)
		line(image, Point(i,0), Point(i,height), Scalar(0,0,0), 1, 1);
	
	
	for (int i = 1; i < posDataObj0.rows; ++i) {
		Point2d lastPosition(i-1, posDataObj0.at<double>(i-1, 0));
		Point2d currentPosition(i, posDataObj0.at<double>(i, 0));
		
		//cout << "Last Pos: " << lastPosition << " Current Pos: " << currentPosition << endl;
		line(image, lastPosition, currentPosition, Scalar(0,0,255), 1, 1);
	}
	
	// Get peaks for the first object
	vector<Point2i> peakPos = findPositionPeaks();
	
	// Create black empty images
	cout << "Peaks detected: " << peakPos.size() << endl;
	for (int i = 0; i < peakPos.size(); ++i) {
		Point2i pos = peakPos.at(i);
		Point2d peak(pos.y, posDataObj0.at<double>(pos.y, 0));
		cout << "pos " << pos << " peak " << peak << endl;
		
		circle(image, peak, 2, Scalar(255,0,0), 2, 8);
	}
	
	imshow("plot0", image);
}


/*void LatencyTool::plotPositionsGraph() {
	
	// Plot positions for the first object
	Mat data;
	for (int i = 0; i < posDataObj0.rows; ++i) {
		Mat row = (Mat_<double>(1,2,CV_64F) << i, posDataObj0.at<double>(i, 0));
		data.push_back(row);
		cout << "row " << row << endl;
	}
	
	Mat plot_result0;
	Ptr<plot::Plot2d> plot;
	plot = plot::createPlot2d(data.col(0), data.col(1));
	plot->setNeedPlotLine(true);
	plot->setPlotLineColor(Scalar(0,0,255));
	plot->render(plot_result0);
	
	// Get peaks for the first object
	vector<Point2i> peakPos = findPositionPeaks();
	
	// Create black empty images
	Mat peaks;
	cout << "Peaks detected: " << peakPos.size() << endl;
	for (int i = 0; i < peakPos.size(); ++i) {
		Point2i pos = peakPos.at(i);
		Mat row = data.row(pos.y);
		Point2f peak(row.at<double>(0,0), row.at<double>(0,1));
		cout << "peak " << peak << endl;
		//drawMarker(peak_result, peak, Scalar(0,0,255), MARKER_STAR, 10, 1);
		
		peaks.push_back(row);
	}
	
	//plot = plot::createPlot2d(peaks.col(1));
	//plot->setPlotLineColor(Scalar(255,0,0));
	//plot->render(plot_result0);
	
	
	// Plot positions for the second object
	data.release();
	for (int i = 0; i < posDataObj1.rows; ++i) {
		Mat row = (Mat_<double>(1,2,CV_64F) << i, posDataObj1.at<double>(i, 0));
		data.push_back(row);
	}
	
	Mat plot_result1;
	plot = plot::createPlot2d(data.col(0), data.col(1));
	plot->render(plot_result1);
	
	//for (int i = 0; i < data.rows; ++i) {
	//	cout << " " << data.row(i) << endl;
	//}

	// Show the plot
	imshow("plot0", plot_result0);
	//imshow("plot1", plot_result1);
	
	
}*/

void LatencyTool::plotVelocityGraph() {
	
	Mat data;
	
	for (int i = 1; i < posDataObj0.rows; ++i) {
		Point2d lastPosition(posDataObj0.at<double>(i-1, 0), posDataObj0.at<double>(i-1, 1));
		Point2d currentPosition(posDataObj0.at<double>(i, 0), posDataObj0.at<double>(i, 1));
		
		//cout << "Last Pos: " << lastPosition << " Current Pos: " << currentPosition << endl;
		
		float dist = sqrt((lastPosition.x - currentPosition.x) * (lastPosition.x - currentPosition.x) + (lastPosition.y - currentPosition.y) * (lastPosition.y - currentPosition.y));
		float velocity = dist/fps;
		
		Mat row;
		row.create(1, 1, CV_64F);
		row.at<double>(0) = velocity;
		data.push_back(row);
	}
	
	plot(data);
}

void LatencyTool::plot(Mat data) {
	Mat plot_result;
	
	Ptr<plot::Plot2d> plot;
	plot = plot::createPlot2d(data);
	plot->render(plot_result);
	
	// Show the plot
	imshow("plot", plot_result);
}
