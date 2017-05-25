// Standard include files
#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>

#include "LatencyTool.h"

#include <iostream>

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

int main(int argc, const char * argv[]) {
	
	// List of tracker types in OpenCV 3.2
	const char *types[] = {"BOOSTING", "MIL", "KCF", "TLD","MEDIANFLOW"};
	vector <string> trackerTypes(types, std::end(types));
	
	// Create a tracker
	string trackingAlg = trackerTypes[2];
	MultiTracker trackers(trackingAlg);
	
	// Container of the tracked objects
	vector<Rect2d> objects;
	
	// Read video
	VideoCapture video("videos/video1.mov");
	
	// Exit if video is not opened
	if(!video.isOpened()) {
		cout << "Could not read video file" << endl;
		return 1;
	}
	
	float width = video.get(CV_CAP_PROP_FRAME_WIDTH);
	float height = video.get(CV_CAP_PROP_FRAME_HEIGHT);
	double fps = video.get(CV_CAP_PROP_FPS);
	
	// Initialise Latency Tool
	LatencyTool* latency = new LatencyTool(fps, width, height);
	//vector<Point2i> peakPos = latency->findPositionPeaks();
	
	// Read first frame
	Mat frame;
	bool ok = video.read(frame);
	
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
			int frames = latency->countFrames();
			float ms = frames*1/fps;
			cout << "Frames: " << frames << " ms: " << ms << endl;
		}
		
		if (k != 255) cout << "Key pressed " << k << endl;
		
	}
	
	int frames = latency->countFrames();
	float ms = frames*1/fps;
	cout << "Frames: " << frames << " ms: " << ms << endl;
 
	return 0;
}
