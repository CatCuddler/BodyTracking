#pragma once

#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

class Peak {

private:
	void nonMaximaSuppression(const cv::Mat& src, cv::Mat& mask, const bool remove_plateaus);
	void nonMinimaSuppression(const cv::Mat& src, cv::Mat& mask, const bool remove_plateaus);
	
public:
	Peak();
	
	void findPeaks(InputArray input, OutputArray output, const float scale = 0.2, const Size& ksize = Size(9, 9), const bool remove_plateus = true);
};
