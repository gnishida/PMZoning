/**
 * Mahalanobis distanceを使って、N次元サンプルをK-meansアルゴリズムでクラスタリングする。
 * クラスタ中心の初期化には、K-means++アルゴリズムを使用する。
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
	int findNearestCenter(const Mat_<double>& sample, const Mat_<double>& mu, double& min_dist);
	int findNearestCenter(const Mat_<double>& sample, const Mat_<double>& mu, const Mat& invCovar);
	int sampleFromCdf(std::vector<double> &cdf);
	int sampleFromPdf(std::vector<double> &pdf);
};

