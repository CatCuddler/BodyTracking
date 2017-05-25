#include "LatencyTool.h"

#include <opencv2/plot.hpp>

#include <iostream>

LatencyTool::LatencyTool(int fps) : fps(fps) {
	
}

void LatencyTool::updatePositions(Point2f point0, Point2f point1) {

	Mat row0 = (Mat_<double>(1,2) << point0.x, point0.y);
	posDataObj0.push_back(row0);
	
	Mat row1 = (Mat_<double>(1,2) << point1.x, point1.y);
	posDataObj1.push_back(row1);
	
}

void LatencyTool::plotPositionsGraph() {
	
	Mat data;
	for (int i = 0; i < posDataObj0.rows; ++i) {
		Mat row = (Mat_<double>(1,2,CV_64F) << i, posDataObj0.at<double>(i, 0));
		data.push_back(row);
	}
	
	Mat plot_result0;
	Ptr<plot::Plot2d> plot;
	plot = plot::createPlot2d(data.col(0), data.col(1));
	plot->render(plot_result0);
	
	data.release();
	for (int i = 0; i < posDataObj1.rows; ++i) {
		Mat row = (Mat_<double>(1,2,CV_64F) << i, posDataObj1.at<double>(i, 0));
		data.push_back(row);
	}
	
	Mat plot_result1;
	plot = plot::createPlot2d(data.col(0), data.col(1));
	plot->render(plot_result1);
	
	/*for (int i = 0; i < data.rows; ++i) {
		cout << " " << data.row(i) << endl;
	 }*/
	
	Mat plot_result;
	plot_result.push_back(plot_result0);
	plot_result.push_back(plot_result1);
	
	// Show the plot
	imshow("plot", plot_result);
	
}

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
