//============================================================================
// Name        : Aia3.h
// Author      : Ronny Haensch
// Version     : 1.0
// Copyright   : -
// Description : header file for second AIA assignment
//============================================================================

#include <iostream>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

class Aia3{

	public:
		// constructor
		Aia3(void){};
		// destructor
		~Aia3(void){};
		
		// processing routine
		// --> some parameters have to be set in this function
		void run(string, string);
		// testing routine
		void test(string, float, float);

	private:
		// --> these functions need to be edited
		void makeFFTObjectMask(const vector<Mat>& templ, double scale, double angle, Mat& fftMask);
		vector<Mat> makeObjectTemplate(const Mat& templateImage, double sigma, double templateThresh);
		vector< vector<Mat> > generalHough(const Mat& gradImage, const vector<Mat>& templ, double scaleSteps, double* scaleRange, double angleSteps, double* angleRange);
		void plotHough(const vector< vector<Mat> >& houghSpace);
		// given functions
		void process(const Mat&, const Mat&, const Mat&);
		Mat makeTestImage(const Mat& temp, double angle, double scale, double* scaleRange);
		Mat rotateAndScale(const Mat& temp, double angle, double scale);
		Mat calcDirectionalGrad(const Mat& image, double sigma);
		void showImage(const Mat& img, string win, double dur);
		Mat circShift(const Mat& in, int dx, int dy);
		void findHoughMaxima(const vector< vector<Mat> >& houghSpace, double objThresh, vector<Scalar>& objList);
		void plotHoughDetectionResult(const Mat& testImage, const vector<Mat>& templ, const vector<Scalar>& objList, double scaleSteps, double* scaleRange, double angleSteps, double* angleRange);
		
};
