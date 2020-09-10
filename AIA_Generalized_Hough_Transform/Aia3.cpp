//============================================================================
// Name        : Aia3.cpp
// Author      : Ronny Haensch
// Version     : 1.0
// Copyright   : -
// Description : 
//============================================================================

#include "Aia3.h"


// creates object template from template image
/*
templateImage:	the template image
sigma:			standard deviation of directional gradient kernel
templateThresh:	threshold for binarization of the template image
return:			the computed template
*/
vector<Mat> Aia3::makeObjectTemplate(const Mat& templateImage, double sigma, double templateThresh) {

	// initialization of the Mat vector and vectors
	vector<Mat> result;
	Mat binaryMask(templateImage.rows, templateImage.cols, CV_32FC1);
	Mat magn(templateImage.rows, templateImage.cols, CV_32FC1);

	// edge extraction
	Mat Gx, Gy;
	Sobel(templateImage, Gx, binaryMask.depth(), 1, 0, 3);
	Sobel(templateImage, Gy, binaryMask.depth(), 0, 1, 3);

	// find magnitude of the binary edge image
	magnitude(Gx, Gy, magn);

	// find maximum in magnitude
	double maxm, minm;
	cv::minMaxLoc(magn, &minm, &maxm);

	// Apply global threshold to the image
	for (int i = 0; i < binaryMask.rows; i++) {
		for (int j = 0; j < binaryMask.cols; j++) {

			if ((magn.at<float>(i, j))>(templateThresh * maxm)) {
				binaryMask.at<float>(i, j) = 1;
			}
			else
			{
				binaryMask.at<float>(i, j) = 0;

			}
		}
	}

	// generate corresponding complex gradients
	Mat complexGrad = calcDirectionalGrad(templateImage, sigma);

	// combine in vector of Mat both binary edge mask and corresponding complex gradients 
	result.push_back(binaryMask);
	result.push_back(complexGrad);

	return result;
}


// creates the fourier-spectrum of the scaled and rotated template
/*
templ:	the object template; binary image in templ[0], complex gradient in templ[1]
scale:	the scale factor to scale the template
angle:	the angle to rotate the template
fftMask:	the generated fourier-spectrum of the template (initialized outside of this function)
*/
void Aia3::makeFFTObjectMask(const vector<Mat>& templ, double scale, double angle, Mat& fftMask) {

	//// initialization of matrices
	//Mat BinaryEdgeMask, ComplexGradient;
	//BinaryEdgeMask = templ[0].clone();
	//ComplexGradient = templ[1].clone();

	//// rotation and scaling called from given function (update phase of gradients)
	//BinaryEdgeMask = rotateAndScale(BinaryEdgeMask, angle, scale);
	//ComplexGradient = rotateAndScale(ComplexGradient, angle, scale);

	//// normalize magnitude of complex Gradient 
	//Mat planes[2];
	//Mat ComplexGrad;
	//split(ComplexGradient, planes);
	//magnitude(planes[0], planes[1], planes[0]);
	//ComplexGrad /= sum(planes[0])[0];

	//// resize objectMask to the image gradient size
	//Mat objectMask(fftMask.rows, fftMask.cols, fftMask.type(), Scalar(0));
	//ComplexGradient.copyTo(objectMask(Rect(Point2i(0, 0), ComplexGradient.size())));

	//// OiOb component wise multiplication
	//for (int i = 0; i < BinaryEdgeMask.rows; ++i) {
	//	for (int j = 0; j < BinaryEdgeMask.cols; ++j) {
	//		objectMask.at<Vec2f>(i, j)[0] = BinaryEdgeMask.at<float>(i, j)*ComplexGradient.at<Vec2f>(i, j)[0];
	//		objectMask.at<Vec2f>(i, j)[1] = BinaryEdgeMask.at<float>(i, j)*ComplexGradient.at<Vec2f>(i, j)[1];
	//	}
	//}

	////call for the circular shift function
	//objectMask = circShift(objectMask, -1*BinaryEdgeMask.rows / 2, -1*BinaryEdgeMask.cols / 2);

	//// fourier transformation
	//dft(objectMask, fftMask, DFT_COMPLEX_OUTPUT);

	// initialization of matrices
	Mat binaryEdge = templ[0].clone();
	Mat complexGradients = templ[1].clone();

	// rotation and scaling called from given function (update phase of gradients)
	binaryEdge = rotateAndScale(binaryEdge, angle, scale);
	complexGradients = rotateAndScale(complexGradients, angle, scale);

	// compute and normalize magnitude of complex Gradient 
	double magnitude = 0;
	for (int i = 0; i < complexGradients.rows; ++i) {
		for (int j = 0; j < complexGradients.cols; ++j) {
			double re = complexGradients.at<Vec2f>(i, j)[0];
			double im = complexGradients.at<Vec2f>(i, j)[1];
			complexGradients.at<Vec2f>(i, j)[0] = re * cos(angle) - im * sin(angle);
			complexGradients.at<Vec2f>(i, j)[1] = re * sin(angle) + im * cos(angle);
			Vec2f gradient = complexGradients.at<Vec2f>(i, j);
			magnitude += norm(gradient);
		}
	}

	complexGradients /= magnitude;

	// resize objectMask to the image gradient size
	Mat objectMask = Mat::zeros(fftMask.rows, fftMask.cols, CV_32FC2);

	// Oi*Ob component wise multiplication
	for (int i = 0; i < binaryEdge.rows; ++i) {
		for (int j = 0; j < binaryEdge.cols; ++j) {
			objectMask.at<Vec2f>(i, j)[0] = binaryEdge.at<float>(i, j)*complexGradients.at<Vec2f>(i, j)[0];
			objectMask.at<Vec2f>(i, j)[1] = binaryEdge.at<float>(i, j)*complexGradients.at<Vec2f>(i, j)[1];
		}
	}

	//call for the circular shift function
	objectMask = circShift(objectMask, -binaryEdge.cols / 2, -binaryEdge.rows / 2);

	// fourier transformation
	dft(objectMask, fftMask, DFT_COMPLEX_OUTPUT);

}

// computes the hough space of the general hough transform
/*
gradImage:	the gradient image of the test image
templ:		the template consisting of binary image and complex-valued directional gradient image
scaleSteps:	scale resolution
scaleRange:	range of investigated scales [min, max]
angleSteps:	angle resolution
angleRange:	range of investigated angles [min, max)
return:		the hough space: outer vector over scales, inner vector of angles
*/
vector< vector<Mat> > Aia3::generalHough(const Mat& gradImage, const vector<Mat>& templ, double scaleSteps, double* scaleRange, double angleSteps, double* angleRange) {


	// initialization hough result
	vector<vector<Mat>> hough;

	//...convert gradImage to the frequency domain: ImageMask
	Mat ImageMask_DFT(gradImage.rows, gradImage.cols, CV_32FC2);
	dft(gradImage, ImageMask_DFT, DFT_COMPLEX_OUTPUT);

	//...convert templ to frequency domain: ObjectMask
	Mat ObjectMask_DFT(gradImage.rows, gradImage.cols, CV_32FC2);

	//...Discretization of teta by dividing the interval [0, 2pi] into A sub-intervals
	double sub_interval_teta = 0;
	sub_interval_teta += angleRange[1] - angleRange[0];
	sub_interval_teta /= angleSteps;


	//...Discretization of scale by dividing the interval [0.5, 2.0]
	double sub_interval_scale = 0;
	sub_interval_scale += scaleRange[1] - scaleRange[0];
	sub_interval_scale /= scaleSteps;


	//...scale dimension
	for (int i = 0; i < scaleSteps; i++) {

		//...update scale
		double scale = scaleRange[0] + i * sub_interval_scale;

		//...orientation matrix
		vector<Mat> Orientation_array;

		//...teta dimension
		for (int j = 0; j < angleSteps; j++) {

			//...update teta
			double angle = angleRange[0] + j * sub_interval_teta;

			//
			makeFFTObjectMask(templ, scale, angle, ObjectMask_DFT);

			//...correlation in the frequency domain
			Mat Correlation_DFT(gradImage.rows, gradImage.cols, CV_32FC2);
			mulSpectrums(ImageMask_DFT, ObjectMask_DFT, Correlation_DFT, 0, true);

			//...correlation back to the spatial domain
			dft(Correlation_DFT, Correlation_DFT, DFT_INVERSE | DFT_SCALE);

			//
			Mat result(Correlation_DFT.rows, Correlation_DFT.cols, CV_32FC1);

			//...spatial dimensions
			for (size_t y = 0; y < gradImage.rows; y++) {

				for (size_t x = 0; x< gradImage.cols; x++) {

					//...absolute value of the correlation matrix
					result.at<float>(y, x) = abs(Correlation_DFT.at<Vec2f>(y, x)[0]);
				}

			}

			Orientation_array.push_back(result);
		}

		hough.push_back(Orientation_array);
	}

	return hough;
}



// shows hough space, eg. as a projection of angle- and scale-dimensions down to a single image
/*
houghSpace:	the hough space as generated by generalHough(..)
*/
void Aia3::plotHough(const vector< vector<Mat> >& houghSpace) {

	Mat Spacehough = Mat::zeros(houghSpace.at(0).at(0).rows, houghSpace.at(0).at(0).cols, CV_32FC1);

	// loop over all dimensions
	for (int scale = 0; scale < houghSpace.size(); ++scale) {
		for (int orientation = 0; orientation < houghSpace.at(scale).size(); ++orientation) {
			Mat fixed = houghSpace.at(scale).at(orientation);
			for (int i = 0; i < fixed.rows; ++i) {
				for (int j = 0; j < fixed.cols; ++j) {

					Spacehough.at<float>(i, j) += fixed.at<float>(i, j);
				}
			}
		}
	}


	showImage(Spacehough, "Hough Space", 0);
	Mat tempImage;
	normalize(Spacehough, tempImage, 0, 255, CV_MINMAX);
	tempImage.convertTo(tempImage, CV_8UC1);
	imwrite("hough_space.png", tempImage);
}



/* *****************************
GIVEN FUNCTIONS
***************************** */

// loads template and test images, sets parameters and calls processing routine
/*
tmplImg:	path to template image
testImg:	path to test image
*/
void Aia3::run(string tmplImg, string testImg) {

	// processing parameter
	double sigma = 1;		// standard deviation of directional gradient kernel
	double templateThresh = 0.3;		// relative threshold for binarization of the template image
										// TO DO !!!
										// ****
										// Set parameters to reasonable values
	double objThresh = 0.53;		// relative threshold for maxima in hough space
	double scaleSteps = 33;		// scale resolution in terms of number of scales to be investigated
	double scaleRange[2];				// scale of angles [min, max]
	scaleRange[0] = 0.5;
	scaleRange[1] = 2;
	double angleSteps = 4;		// angle resolution in terms of number of angles to be investigated
	double angleRange[2];				// range of angles [min, max)
	angleRange[0] = 0;
	angleRange[1] = 2 * CV_PI;
	// ****

	Mat params = (Mat_<float>(1, 9) << sigma, templateThresh, objThresh, scaleSteps, scaleRange[0], scaleRange[1], angleSteps, angleRange[0], angleRange[1]);

	// load template image as gray-scale, paths in argv[1]
	Mat templateImage = imread(tmplImg, 0);
	if (!templateImage.data) {
		cerr << "ERROR: Cannot load template image from\n" << tmplImg << endl;
		cerr << "Press enter..." << endl;
		cin.get();
		exit(-1);
	}
	// convert 8U to 32F
	templateImage.convertTo(templateImage, CV_32FC1);
	// show template image
	showImage(templateImage, "Template image", 0);

	// load test image
	Mat testImage = imread(testImg, 0);
	if (!testImage.data) {
		cerr << "ERROR: Cannot load test image from\n" << testImg << endl;
		cerr << "Press enter..." << endl;
		cin.get();
		exit(-1);
	}
	// and convert it from 8U to 32F
	testImage.convertTo(testImage, CV_32FC1);
	// show test image
	showImage(testImage, "testImage", 0);

	// start processing
	process(templateImage, testImage, params);

}

// loads template and create test image, sets parameters and calls processing routine
/*
tmplImg:	path to template image
angle:		rotation angle in degree of the test object
scale:		scale of the test object
*/
void Aia3::test(string tmplImg, float angle, float scale) {

	// angle to rotate template image (in radian)
	double testAngle = angle / 180.*CV_PI;
	// scale to scale template image
	double testScale = scale;

	// processing parameter
	double sigma = 1;		// standard deviation of directional gradient kernel
	double templateThresh = 0.7;		// relative threshold for binarization of the template image
	double objThresh = 0.85;		// relative threshold for maxima in hough space
	double scaleSteps = 3;		// scale resolution in terms of number of scales to be investigated
	double scaleRange[2];				// scale of angles [min, max]
	scaleRange[0] = 1;
	scaleRange[1] = 2;
	double angleSteps = 12;		// angle resolution in terms of number of angles to be investigated
	double angleRange[2];				// range of angles [min, max)
	angleRange[0] = 0;
	angleRange[1] = 2 * CV_PI;

	Mat params = (Mat_<float>(1, 9) << sigma, templateThresh, objThresh, scaleSteps, scaleRange[0], scaleRange[1], angleSteps, angleRange[0], angleRange[1]);

	// load template image as gray-scale, paths in argv[1]
	Mat templateImage = imread(tmplImg, 0);
	if (!templateImage.data) {
		cerr << "ERROR: Cannot load template image from\n" << tmplImg << endl;
		cerr << "Press enter..." << endl;
		cin.get();
		exit(-1);
	}
	// convert 8U to 32F
	templateImage.convertTo(templateImage, CV_32FC1);
	// show template image
	showImage(templateImage, "Template Image", 0);

	// generate test image
	Mat testImage = makeTestImage(templateImage, testAngle, testScale, scaleRange);
	// show test image
	showImage(testImage, "Test Image", 0);

	// start processing
	process(templateImage, testImage, params);
}

void Aia3::process(const Mat& templateImage, const Mat& testImage, const Mat& params) {

	// processing parameter
	double sigma = params.at<float>(0);		// standard deviation of directional gradient kernel
	double templateThresh = params.at<float>(1);		// relative threshold for binarization of the template image
	double objThresh = params.at<float>(2);		// relative threshold for maxima in hough space
	double scaleSteps = params.at<float>(3);		// scale resolution in terms of number of scales to be investigated
	double scaleRange[2];								// scale of angles [min, max]
	scaleRange[0] = params.at<float>(4);
	scaleRange[1] = params.at<float>(5);
	double angleSteps = params.at<float>(6);		// angle resolution in terms of number of angles to be investigated
	double angleRange[2];								// range of angles [min, max)
	angleRange[0] = params.at<float>(7);
	angleRange[1] = params.at<float>(8);

	// calculate directional gradient of test image as complex numbers (two channel image)
	Mat gradImage = calcDirectionalGrad(testImage, sigma);

	// generate template from template image
	// templ[0] == binary image
	// templ[0] == directional gradient image
	vector<Mat> templ = makeObjectTemplate(templateImage, sigma, templateThresh);

	// show binary image
	showImage(templ[0], "Binary part of template", 0);

	// perfrom general hough transformation
	vector< vector<Mat> > houghSpace = generalHough(gradImage, templ, scaleSteps, scaleRange, angleSteps, angleRange);

	// plot hough space (max over angle- and scale-dimension)
	plotHough(houghSpace);

	// find maxima in hough space
	vector<Scalar> objList;
	findHoughMaxima(houghSpace, objThresh, objList);

	// print found objects on screen
	cout << "Number of objects: " << objList.size() << endl;
	int i = 0;
	for (vector<Scalar>::const_iterator it = objList.begin(); it != objList.end(); it++, i++) {
		cout << i << "\tScale:\t" << (scaleRange[1] - scaleRange[0]) / (scaleSteps - 1)*(*it).val[0] + scaleRange[0];

		cout << "\tAngle:\t" << ((angleRange[1] - angleRange[0]) / (angleSteps)*(*it).val[1] + angleRange[0]) / CV_PI * 180;
		cout << "\tPosition:\t(" << (*it).val[2] << ", " << (*it).val[3] << " )" << endl;
	}

	// show final detection result
	plotHoughDetectionResult(testImage, templ, objList, scaleSteps, scaleRange, angleSteps, angleRange);

}
// computes directional gradients
/*
image:	the input image
sigma:	standard deviation of the kernel
return:	the two-channel gradient image
*/
Mat Aia3::calcDirectionalGrad(const Mat& image, double sigma) {

	// compute kernel size
	int ksize = max(sigma * 3, 3.);
	if (ksize % 2 == 0)  ksize++;
	double mu = ksize / 2.0;

	// generate kernels for x- and y-direction
	double val, sum = 0;
	Mat kernel(ksize, ksize, CV_32FC1);
	//Mat kernel_y(ksize, ksize, CV_32FC1);
	for (int i = 0; i<ksize; i++) {
		for (int j = 0; j<ksize; j++) {
			val = pow((i + 0.5 - mu) / sigma, 2);
			val += pow((j + 0.5 - mu) / sigma, 2);
			val = exp(-0.5*val);
			sum += val;
			kernel.at<float>(i, j) = -(j + 0.5 - mu)*val;
		}
	}
	kernel /= sum;

	// use those kernels to compute gradient in x- and y-direction independently
	vector<Mat> grad(2);
	filter2D(image, grad[0], -1, kernel);
	filter2D(image, grad[1], -1, kernel.t());


	// combine both real-valued gradient images to a single complex-valued image
	Mat output;

	merge(grad, output);

	return output;
}

// rotates and scales a given image
/*
image:	the image to be scaled and rotated
angle:	rotation angle in radians
scale:	scaling factor
return:	transformed image
*/
Mat Aia3::rotateAndScale(const Mat& image, double angle, double scale) {

	// create transformation matrices
	// translation to origin
	Mat T = Mat::eye(3, 3, CV_32FC1);
	T.at<float>(0, 2) = -image.cols / 2.0;
	T.at<float>(1, 2) = -image.rows / 2.0;


	// rotation
	Mat R = Mat::eye(3, 3, CV_32FC1);
	R.at<float>(0, 0) = cos(angle);
	R.at<float>(0, 1) = -sin(angle);
	R.at<float>(1, 0) = sin(angle);
	R.at<float>(1, 1) = cos(angle);

	// scale
	Mat S = Mat::eye(3, 3, CV_32FC1);
	S.at<float>(0, 0) = scale;
	S.at<float>(1, 1) = scale;

	// combine
	Mat H = R * S*T;

	// compute corners of warped image
	Mat corners(1, 4, CV_32FC2);
	corners.at<Vec2f>(0, 0) = Vec2f(0, 0);
	corners.at<Vec2f>(0, 1) = Vec2f(0, image.rows);
	corners.at<Vec2f>(0, 2) = Vec2f(image.cols, 0);
	corners.at<Vec2f>(0, 3) = Vec2f(image.cols, image.rows);
	perspectiveTransform(corners, corners, H);

	// compute size of resulting image and allocate memory
	float x_start = min(min(corners.at<Vec2f>(0, 0)[0], corners.at<Vec2f>(0, 1)[0]), min(corners.at<Vec2f>(0, 2)[0], corners.at<Vec2f>(0, 3)[0]));
	float x_end = max(max(corners.at<Vec2f>(0, 0)[0], corners.at<Vec2f>(0, 1)[0]), max(corners.at<Vec2f>(0, 2)[0], corners.at<Vec2f>(0, 3)[0]));
	float y_start = min(min(corners.at<Vec2f>(0, 0)[1], corners.at<Vec2f>(0, 1)[1]), min(corners.at<Vec2f>(0, 2)[1], corners.at<Vec2f>(0, 3)[1]));
	float y_end = max(max(corners.at<Vec2f>(0, 0)[1], corners.at<Vec2f>(0, 1)[1]), max(corners.at<Vec2f>(0, 2)[1], corners.at<Vec2f>(0, 3)[1]));

	// create translation matrix in order to copy new object to image center
	T.at<float>(0, 0) = 1;
	T.at<float>(1, 1) = 1;
	T.at<float>(2, 2) = 1;
	T.at<float>(0, 2) = (x_end - x_start + 1) / 2.0;
	T.at<float>(1, 2) = (y_end - y_start + 1) / 2.0;

	// change homography to take necessary translation into account
	H = T * H;
	// warp image and copy it to output image
	Mat output;
	warpPerspective(image, output, H, Size(x_end - x_start + 1, y_end - y_start + 1), CV_INTER_LINEAR);

	return output;

}

// generates the test image as a transformed version of the template image
/*
temp:		    the template image
angle:	    rotation angle
scale:	    scaling factor
scaleRange:	scale range [min,max], used to determine the image size
*/
Mat Aia3::makeTestImage(const Mat& temp, double angle, double scale, double* scaleRange) {

	// rotate and scale template image
	Mat small = rotateAndScale(temp, angle, scale);

	// create empty test image
	Mat testImage = Mat::zeros(temp.rows*scaleRange[1] * 2, temp.cols*scaleRange[1] * 2, CV_32FC1);
	// copy new object into test image
	Mat tmp;
	Rect roi;
	roi = Rect((testImage.cols - small.cols)*0.5, (testImage.rows - small.rows)*0.5, small.cols, small.rows);
	tmp = Mat(testImage, roi);
	small.copyTo(tmp);

	return testImage;
}

// shows the detection result of the hough transformation
/*
testImage:	the test image, where objects were searched (and hopefully found)
templ:		the template consisting of binary image and complex-valued directional gradient image
objList:		list of objects as defined by findHoughMaxima(..)
scaleSteps:	scale resolution
scaleRange:	range of investigated scales [min, max]
angleSteps:	angle resolution
angleRange:	range of investigated angles [min, max)
*/
void Aia3::plotHoughDetectionResult(const Mat& testImage, const vector<Mat>& templ, const vector<Scalar>& objList, double scaleSteps, double* scaleRange, double angleSteps, double* angleRange) {

	// some matrices to deal with color
	Mat red = testImage.clone();
	Mat green = testImage.clone();
	Mat blue = testImage.clone();
	Mat tmp = Mat::zeros(testImage.rows, testImage.cols, CV_32FC1);

	// scale and angle of current object
	double scale, angle;

	// for all objects
	for (vector<Scalar>::const_iterator it = objList.begin(); it != objList.end(); it++) {
		// compute scale and angle of current object
		scale = (scaleRange[1] - scaleRange[0]) / (scaleSteps - 1)*(*it).val[0] + scaleRange[0];
		angle = ((angleRange[1] - angleRange[0]) / (angleSteps)*(*it).val[1] + angleRange[0]);

		// use scale and angle in order to generate new binary mask of template
		Mat binMask = rotateAndScale(templ[0], angle, scale);

		// perform boundary checks
		Rect binArea = Rect(0, 0, binMask.cols, binMask.rows);
		Rect imgArea = Rect((*it).val[2] - binMask.cols / 2., (*it).val[3] - binMask.rows / 2, binMask.cols, binMask.rows);
		if ((*it).val[2] - binMask.cols / 2 < 0) {
			binArea.x = abs((*it).val[2] - binMask.cols / 2);
			binArea.width = binMask.cols - binArea.x;
			imgArea.x = 0;
			imgArea.width = binArea.width;
		}
		if ((*it).val[3] - binMask.rows / 2 < 0) {
			binArea.y = abs((*it).val[3] - binMask.rows / 2);
			binArea.height = binMask.rows - binArea.y;
			imgArea.y = 0;
			imgArea.height = binArea.height;
		}
		if ((*it).val[2] - binMask.cols / 2 + binMask.cols >= tmp.cols) {
			binArea.width = binMask.cols - ((*it).val[2] - binMask.cols / 2 + binMask.cols - tmp.cols);
			imgArea.width = binArea.width;
		}
		if ((*it).val[3] - binMask.rows / 2 + binMask.rows >= tmp.rows) {
			binArea.height = binMask.rows - ((*it).val[3] - binMask.rows / 2 + binMask.rows - tmp.rows);
			imgArea.height = binArea.height;
		}
		// copy this object instance in new image of correct size
		tmp.setTo(0);
		Mat binRoi = Mat(binMask, binArea);
		Mat imgRoi = Mat(tmp, imgArea);
		binRoi.copyTo(imgRoi);

		// delete found object from original image in order to reset pixel values with red (which are white up until now)
		binMask = 1 - binMask;
		imgRoi = Mat(red, imgArea);
		multiply(imgRoi, binRoi, imgRoi);
		imgRoi = Mat(green, imgArea);
		multiply(imgRoi, binRoi, imgRoi);
		imgRoi = Mat(blue, imgArea);
		multiply(imgRoi, binRoi, imgRoi);

		// change red channel
		red = red + tmp * 255;
	}
	// generate color image
	vector<Mat> color;
	color.push_back(blue);
	color.push_back(green);
	color.push_back(red);
	Mat display;
	merge(color, display);
	// display color image
	showImage(display, "result", 0);
	// save color image
	imwrite("detectionResult.png", display);
}

// seeks for local maxima within the hough space
/*
a local maxima has to be larger than all its 8 spatial neighbors, as well as the largest value at this position for all scales and orientations
houghSpace:	the computed hough space
objThresh:	relative threshold for maxima in hough space
objList:	list of detected objects
*/
void Aia3::findHoughMaxima(const vector< vector<Mat> >& houghSpace, double objThresh, vector<Scalar>& objList) {

	// get maxima over scales and angles
	Mat maxImage = Mat::zeros(houghSpace.at(0).at(0).rows, houghSpace.at(0).at(0).cols, CV_32FC1);

	for (vector< vector<Mat> >::const_iterator it = houghSpace.begin(); it != houghSpace.end(); it++) {

		for (vector<Mat>::const_iterator img = (*it).begin(); img != (*it).end(); img++) {
			max(*img, maxImage, maxImage);
		}
	}

	// get global maxima
	double min, max;
	minMaxLoc(maxImage, &min, &max);

	// define threshold
	double threshold = objThresh * max;

	// spatial non-maxima suppression
	Mat bin = Mat(houghSpace.at(0).at(0).rows, houghSpace.at(0).at(0).cols, CV_32FC1, -1);
	for (int y = 0; y<maxImage.rows; y++) {
		for (int x = 0; x<maxImage.cols; x++) {
			// init
			bool localMax = true;
			// check neighbors
			for (int i = -1; i <= 1; i++) {
				int new_y = y + i;
				if ((new_y < 0) || (new_y >= maxImage.rows)) {
					continue;
				}
				for (int j = -1; j <= 1; j++) {
					int new_x = x + j;
					if ((new_x < 0) || (new_x >= maxImage.cols)) {
						continue;
					}
					if (maxImage.at<float>(new_y, new_x) > maxImage.at<float>(y, x)) {
						localMax = false;
						break;
					}
				}
				if (!localMax)
					break;
			}
			// check if local max is larger than threshold
			if ((localMax) && (maxImage.at<float>(y, x) > threshold)) {
				bin.at<float>(y, x) = maxImage.at<float>(y, x);
			}
		}
	}

	// loop through hough space after non-max suppression and add objects to object list
	double scale, angle;
	scale = 0;
	for (vector< vector<Mat> >::const_iterator it = houghSpace.begin(); it != houghSpace.end(); it++, scale++) {
		angle = 0;
		for (vector<Mat>::const_iterator img = (*it).begin(); img != (*it).end(); img++, angle++) {
			for (int y = 0; y<bin.rows; y++) {
				for (int x = 0; x<bin.cols; x++) {
					if ((*img).at<float>(y, x) == bin.at<float>(y, x)) {
						// create object list entry consisting of scale, angle, and position where object was detected
						Scalar cur;
						cur.val[0] = scale;
						cur.val[1] = angle;
						cur.val[2] = x;
						cur.val[3] = y;
						objList.push_back(cur);
					}
				}
			}
		}
	}
}

// shows the image
/*
img:	the image to be displayed
win:	the window name
dur:	wait number of ms or until key is pressed
*/
void Aia3::showImage(const Mat& img, string win, double dur) {

	// use copy for normalization
	Mat tempDisplay;
	if (img.channels() == 1)
		normalize(img, tempDisplay, 0, 255, CV_MINMAX);
	else
		tempDisplay = img.clone();

	tempDisplay.convertTo(tempDisplay, CV_8UC1);

	// create window and display omage
	namedWindow(win.c_str(), 0);
	imshow(win.c_str(), tempDisplay);
	// wait
	if (dur >= 0) cvWaitKey(dur);
	// be tidy
	destroyWindow(win.c_str());

}

// Performes a circular shift in (dx,dy) direction
/*
in:		input matrix
out:	circular shifted matrix
dx:		shift in x-direction
dy:		shift in y-direction
*/
Mat Aia3::circShift(const Mat& in, int dx, int dy) {

	Mat tmp = Mat::zeros(in.rows, in.cols, in.type());

	int x, y, new_x, new_y;

	for (y = 0; y<in.rows; y++) {

		// calulate new y-coordinate
		new_y = y + dy;
		if (new_y<0)
			new_y = new_y + in.rows;
		if (new_y >= in.rows)
			new_y = new_y - in.rows;

		for (x = 0; x<in.cols; x++) {

			// calculate new x-coordinate
			new_x = x + dx;
			if (new_x<0)
				new_x = new_x + in.cols;
			if (new_x >= in.cols)
				new_x = new_x - in.cols;

			tmp.at<Vec2f>(new_y, new_x) = in.at<Vec2f>(y, x);

		}
	}
	return tmp;
}
