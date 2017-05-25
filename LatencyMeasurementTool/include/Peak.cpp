#include "Peak.h"

Peak::Peak() {}

void Peak::nonMaximaSuppression(const Mat& src, Mat& mask, const bool remove_plateaus) {
	// Find pixels that are equal to the local neighborhood not maximum (including 'plateaus')
	cv::dilate(src, mask, cv::Mat());
	compare(src, mask, mask, CMP_GE);
	
	// Optionally filter out pixels that are equal to the local minimum ('plateaus')
	if (remove_plateaus) {
		cv::Mat non_plateau_mask;
		cv::erode(src, non_plateau_mask, cv::Mat());
		cv::compare(src, non_plateau_mask, non_plateau_mask, CMP_GT);
		cv::bitwise_and(mask, non_plateau_mask, mask);
	}
}

void Peak::nonMinimaSuppression(const cv::Mat& src, cv::Mat& mask, const bool remove_plateaus) {
	// Find pixels that are equal to the local neighborhood not minimum (including 'plateaus')
	cv::erode(src, mask, cv::Mat());
	cv::compare(src, mask, mask, CMP_LE);
	
	// Optionally filter out pixels that are equal to the local maximum ('plateaus')
	if (remove_plateaus) {
		cv::Mat non_plateau_mask;
		cv::dilate(src, non_plateau_mask, cv::Mat());
		cv::compare(src, non_plateau_mask, non_plateau_mask, CMP_LT);
		cv::bitwise_and(mask, non_plateau_mask, mask);
	}
}

// Function that finds the peaks of a given hist image
void Peak::findPeaks(InputArray input, OutputArray output, const float scale, const Size& ksize, const bool remove_plateus) {
	Mat mat = input.getMat();
	// Check the matrix type
	CV_Assert(mat.type() == CV_32F);
	
	// Find the min and max values
	double min_val, max_val;
	minMaxLoc(mat, &min_val, &max_val);
	
	Mat minMask, maxMask;
	GaussianBlur(mat, mat, ksize, 0); // Smooth a bit in order to obtain better result
	nonMinimaSuppression(mat, minMask, remove_plateus); // Extract local minima
	nonMaximaSuppression(mat, maxMask, remove_plateus); // Extract local maxima
	
	Mat mask;
	compare(maxMask, minMask, mask, CMP_NE);
	
	vector<Point> maxima;   // Output, locations of non-zero pixels
	cv::findNonZero(mask, maxima);
	
	for(vector<Point>::iterator it = maxima.begin(); it != maxima.end();) {
		Point pnt = *it;
		float val = mat.at<float>(pnt.y);
		
		// Filter peaks
		if (val > max_val * scale)
			++it;
		else if (val < -(min_val * scale))
			++it;
		else
			it = maxima.erase(it);
	}
	
	Mat(maxima).copyTo(output);
}
