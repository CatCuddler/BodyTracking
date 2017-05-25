#include "Peak.h"

#include <iostream>

Peak::Peak() {
	
}

void Peak::nonMaximaSuppression(const cv::Mat& src, cv::Mat& mask, const bool remove_plateaus) {
	// find pixels that are equal to the local neighborhood not maximum (including 'plateaus')
	cv::dilate(src, mask, cv::Mat());
	compare(src, mask, mask, CMP_GE);
	
	// optionally filter out pixels that are equal to the local minimum ('plateaus')
	if (remove_plateaus) {
		cv::Mat non_plateau_mask;
		cv::erode(src, non_plateau_mask, cv::Mat());
		cv::compare(src, non_plateau_mask, non_plateau_mask, CMP_GT);
		cv::bitwise_and(mask, non_plateau_mask, mask);
	}
}

void Peak::nonMinimaSuppression(const cv::Mat& src, cv::Mat& mask, const bool remove_plateaus) {
	// find pixels that are equal to the local neighborhood not maximum (including 'plateaus')
	cv::erode(src, mask, cv::Mat());
	cv::compare(src, mask, mask, CMP_LE);
	
	// optionally filter out pixels that are equal to the local minimum ('plateaus')
	if (remove_plateaus) {
		cv::Mat non_plateau_mask;
		cv::dilate(src, non_plateau_mask, cv::Mat());
		cv::compare(src, non_plateau_mask, non_plateau_mask, CMP_LT);
		cv::bitwise_and(mask, non_plateau_mask, mask);
	}
}

//Mat data = (Mat_<float>(1,10,CV_32F) << 0, -1, -20, -1, 0, 1, 2, 30, 2, 0, -1);


// function that finds the peaks of a given hist image
void Peak::findPeaks(InputArray _src, OutputArray _idx, const float scale, const Size& ksize, const bool remove_plateus) {
	Mat hist = _src.getMat();
	// die if histogram image is not the correct type
	CV_Assert(hist.type() == CV_32F);
	
	// find the min and max values of the hist image
	double min_val, max_val;
	minMaxLoc(hist, &min_val, &max_val);
	cout << "min " << min_val << " max " << max_val << endl;
	
	Mat maxMask, minMask;
	//GaussianBlur(hist, hist, ksize, 0); // smooth a bit in order to obtain better result
	nonMaximaSuppression(hist, maxMask, remove_plateus); // extract local maxima
	nonMinimaSuppression(hist, minMask, remove_plateus); // extract local minima
	
	Mat mask;
	compare(maxMask, minMask, mask, CMP_NE);
	cout << " " << maxMask << " " << minMask << " " << mask << endl;
	
	vector<Point> maxima;   // output, locations of non-zero pixels
	cv::findNonZero(mask, maxima);
	
	for(vector<Point>::iterator it = maxima.begin(); it != maxima.end();) {
		Point pnt = *it;
		float val = hist.at<float>(/*pnt.x, */pnt.y);
		//float val = hist.at<float>(/*pnt.x, */pnt.x);
		
		cout << " pnt " << pnt<< " val " << val << " max_val*scale " << max_val * scale << endl;
		
		// filter peaks
		if (val > max_val * scale)
			++it;
		else if (val < -(min_val * scale))
			++it;
		else
			it = maxima.erase(it);
	}
	
	Mat(maxima).copyTo(_idx);
}
