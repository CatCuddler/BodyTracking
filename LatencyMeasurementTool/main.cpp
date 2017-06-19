// Standard include files
#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>

#include "LatencyTool.h"

#include <iostream>
#include <sstream>

using namespace cv;
using namespace std;

// Convert to string
#define SSTR( x ) static_cast< std::ostringstream & >( \
( std::ostringstream() << std::dec << x ) ).str()

vector<Point2f> getPositions(vector<Rect2d> objects) {
	vector<Point2f> positions;
	
	for(unsigned i = 0; i < objects.size(); ++i) {
		Rect2d obj = objects[i];
		float xPos = obj.x + obj.width/2.0f;
		float yPos = obj.y + obj.height/2.0f;
		Point2f pos(xPos, yPos);
		
		//cout << "Pos of the object " << i << ": (" << pos.x << "," << pos.y << ")" << endl;
		positions.push_back(pos);
	}
	
	return positions;
}

void displayLatency(LatencyTool* latency, float videoFPS) {
	float frames = latency->countFrames();
	float ms = frames * 1000 / videoFPS;
	cout << "Frames: " << frames << " ms: " << ms << endl;
}

/*int main(int argc, const char* argv[]) {
	const char* fileName = argv[1];
	ifstream inputFile(fileName);
	string currentLine;
	vector< vector<float> > data;
	while(getline(inputFile, currentLine)) {
		vector<float> values;
		stringstream temp(currentLine);
		string singleVal;
		while(getline(temp, singleVal, ',')) {
			values.push_back(atof(singleVal.c_str()));
		}
		data.push_back(values);
	}
	
	// Add data into Mat element
	Mat vec = Mat::zeros((int)data.size(), (int)data[0].size(), CV_32F);
	for (int rows = 0; rows < (int)data.size(); ++rows) {
		for (int cols = 0; cols < (int)data[0].size(); ++cols) {
			vec.at<float>(rows, cols) = data[rows][cols];
			//cout << " " << vec.at<float>(rows, cols) << endl;
		}
	}
	
	float FPS = 240.0;
	LatencyTool* latency = new LatencyTool(FPS, 0, 0, "");
	float frames = latency->getLatencyFromPositionMatrix(vec);
	
	float ms = frames * 1000 / FPS;
	cout << "Frames: " << frames << " ms: " << ms << endl;
}*/

int main(int argc, const char* argv[]) {
	
	// List of tracker types in OpenCV 3.2
	const char *types[] = {"BOOSTING", "MIL", "KCF", "TLD", "MEDIANFLOW"};
	vector <string> trackerTypes(types, std::end(types));
	
	// Create a tracker
	string trackingAlg = trackerTypes[2];
	MultiTracker trackers(trackingAlg);
	
	// Container of the tracked objects
	vector<Rect2d> objects;
	
	// Read video
	std::stringstream videoPath; // Name of the video
	const char* videoName = argv[1];
	videoPath  << "videos/" << videoName;
	cout << "video name: " << videoPath.str() << endl;
	VideoCapture video(videoPath.str());
	
	// Exit if video is not opened
	if(!video.isOpened()) {
		cout << "Could not read video file" << endl;
		return 1;
	}
	
	float videoWidth = video.get(CV_CAP_PROP_FRAME_WIDTH);
	float videoHeight = video.get(CV_CAP_PROP_FRAME_HEIGHT);
	double videoFPS = video.get(CV_CAP_PROP_FPS);
	cout << "width " << videoWidth << " height " << videoHeight << " fps " << videoFPS << endl;
	
	int len = strlen(videoName);
	string name(videoName, len - 4);
	
	// Initialise Latency Tool
	LatencyTool* latency = new LatencyTool(videoFPS, videoWidth, videoHeight, name);
	//vector<Point2i> peakPos = latency->findPositionPeaks();
	
	// Read first frame
	Mat frame;
	bool ok = video.read(frame);
	
	video.set(CV_CAP_PROP_POS_FRAMES, 50);
	video.retrieve(frame, CV_CAP_OPENNI_BGR_IMAGE);
	
	// Define initial boundibg box (one for the marker, one for the VR object)
	selectROI("tracker", frame, objects, false);
	
	// Quit when the tracked object(s) is not provided
	if(objects.size() != 2)
		return 0;
	
	// Initialize the tracker
	trackers.add(frame, objects);
	
	while(video.read(frame)) {
		
		// Start timer
		double timer = (double)getTickCount();
		
		// Update the tracking result
		bool ok = trackers.update(frame);
		
		// Calculate Frames per second (FPS)
		float fps = getTickFrequency() / ((double)getTickCount() - timer);
		
		
		if (ok) {
			// Tracking success : Draw the tracked object
			for(unsigned i = 0; i < trackers.objects.size(); ++i)
				rectangle(frame, trackers.objects[i], Scalar( 255, 0, 0 ), 2, 1 );
			
			// Retrieve a vector of points with the (x,y) location of the objects
			vector<Point2f> positions = getPositions(trackers.objects);
			
			// Draw the center of the bounding boxes
			for(unsigned i = 0; i < positions.size(); ++i)
				circle(frame, positions[i], 1, Scalar( 0, 0, 255 ), 2, 1 );
			
			//int currentFrameNum = video.get(CV_CAP_PROP_POS_FRAMES);
			latency->updatePositions(positions[0], positions[1]);
			
		} else {
			// Tracking failure detected.
			putText(frame, "Tracking failure detected", Point(100,80), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(0,0,255),2);
		}
		
		// Display tracker type on frame
		putText(frame, trackingAlg + " Tracker", Point(100,20), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(50,170,50),2);
		
		// Display FPS on frame
		putText(frame, "FPS : " + SSTR(int(fps)), Point(100,50), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(50,170,50), 2);
		
		// Display frame.
		imshow("Tracking", frame);
		
		int k = waitKey(1);
		
		if(k == 27) {
			// Exit if ESC pressed.
			break;
		}
		
		if(k == 112) {
			// P
			latency->plotPositionsGraph();
		}
		
		if (k == 108) {
			// L
			displayLatency(latency, videoFPS);
		}
		
		if (k != 255) cout << "Key pressed " << k << endl;
		
	}
	
	displayLatency(latency, videoFPS);
	
	latency->plotPositionsGraph();
	
	waitKey(0);
 
	return 0;
}
