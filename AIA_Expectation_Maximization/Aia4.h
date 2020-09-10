//============================================================================
// Name        : Aia4.h
// Author      : Ronny Haensch
// Version     : 1.0
// Copyright   : -
// Description : header file for fourth AIA assignment
//============================================================================

#include <iostream>
#include <stdio.h>
#include <limits>

#include <list>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

struct comp {Mat mean; Mat covar; double weight;};

class Aia5{

	public:
		// constructor
		Aia5(void){};
		// destructor
		~Aia5(void){};
		
		// processing routine
		void run(void);
		void test(void);
		
	private:
		// functions to be written
		Mat calcCompLogL(const vector<struct comp*>& model, const Mat& features);
		Mat calcMixtureLogL(const vector<struct comp*>& model, const Mat& features);
		Mat gmmEStep(const vector<struct comp*>& model, const Mat& features);
		void gmmMStep(vector<struct comp*>& model, const Mat& features, const Mat& posterior);

		// given functions
		void initNewComponent(vector<struct comp*>& model, const Mat& features);
		void plotGMM(const vector<struct comp*>& model, const Mat& features);
		void trainGMM(const Mat& data, int numberOfComponents, vector<struct comp*>& model);
		void readImageDatabase(string dataPath, vector<Mat>& db);
		void genFeatureProjection(const vector<Mat>& imgDB, vector<PCA>& featSpace, int vectLen);

};
