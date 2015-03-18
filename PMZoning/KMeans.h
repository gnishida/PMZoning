/**
 * Mahalanobis distanceを使って、N次元サンプルをK-meansアルゴリズムでクラスタリングする。
 *
 * @author	Gen Nishida
 * @date	3/17/2015
 * @version	1.0
 */

#pragma once

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <vector>

using namespace cv;
using namespace std;

class KMeans {
private:
	int dimensions;
	int num_clusters;


public:
	KMeans(int dimensions, int num_clusters);

	void cluster(Mat_<double> samples, int max_iterations, Mat_<double>& mu, vector<int>& groups);

private:
	void computeMinMax(Mat_<double> samples, vector<double>& mins, vector<double>& maxs);
};

