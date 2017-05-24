#include "LatencyTool.h"

#include <opencv2/plot.hpp>

#include <iostream>

LatencyTool::LatencyTool(int fps) : fps(fps) {
	
}

void LatencyTool::updatePositions(vector<Point2f> positions) {

	positionsObj0.push_back(positions[0]);
	positionsObj1.push_back(positions[1]);
	
	Mat row;
	row.create(1, 1, CV_64F);
	row.at<double>(0) = positions[0].x;
	posDataX.push_back(row);
	
	row.at<double>(1) = positions[1].x;
	posDataY.push_back(row);

	//for(unsigned i = 0; i < positions.size(); ++i)
	//	cout << " " << positions[i] << endl;
	
}

void LatencyTool::plotPositionsGraph() {

	for (int i = 0; i < posDataX.rows; ++i)
		cout << " " << posDataX.row(i) << " " <<  posDataY.row(i) << endl;
	
	Mat plot_result;
	
	Ptr<plot::Plot2d> plot;
	plot = plot::createPlot2d(posDataX);
	plot->render(plot_result);
	
	// Show the plot
	imshow("plot", plot_result);
}

void LatencyTool::plotVelocityGraph() {
	
	Mat data;
	
	for (int i = 1; i < positionsObj0.size(); ++i) {
		Point2f lastPosition = positionsObj0[i-1];
		Point2f currentPosition = positionsObj0[i];
		
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
