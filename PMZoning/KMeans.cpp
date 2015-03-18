#include "KMeans.h"
#include <assert.h>

KMeans::KMeans(int dimensions, int num_clusters) {
	this->dimensions = dimensions;
	this->num_clusters = num_clusters;
}

/**
 * K-meansアルゴリズムにより、サンプルをクラスタリングする。
 *
 * @param samples		サンプルデータ
 * @param mu			クラスタの中心
 * @param groups		各サンプルが属するクラスタID
 */
void KMeans::cluster(Mat_<double> samples, int max_iterations, Mat_<double>& mu, vector<int>& groups) {
	assert(samples.cols == dimensions);

	mu = Mat_<double>(num_clusters, dimensions);
	groups.resize(samples.rows, -1);

	// サンプルの共分散行列を計算する
	Mat covar, mean;
	calcCovarMatrix(samples, covar, mean, CV_COVAR_NORMAL | CV_COVAR_ROWS);
	covar = covar / (samples.rows - 1);

	// 共分散行列の逆行列を計算する
	Mat invCovar;
	cv::invert(covar, invCovar, DECOMP_SVD);

	// 初期クラスタリング（K-means++アルゴリズムで、初期クラスタ中心を決定する）
	{
		int s = (double)rand() / RAND_MAX * samples.rows;
		Mat_<double> temp = mu.rowRange(0, 1);
		samples.row(s).copyTo(temp);

		for (int j = 1; j < num_clusters; ++j) {
			Mat_<double> mu2 = mu.rowRange(0, j);
			
			vector<double> pdf;
			for (int i = 0; i < samples.rows; ++i) {
				double dist;
				int group_id = findNearestCenter(samples.row(i), mu2, invCovar, dist);
				pdf.push_back(dist * dist);
			}

			int s = sampleFromPdf(pdf);
			Mat_<double> temp = mu.rowRange(j, j + 1);
			samples.row(s).copyTo(temp);
		}
	}

	{
		// 各サンプルに最も近いクラスタを求める
		vector<int> num_members(num_clusters, 0);
		for (int i = 0; i < samples.rows; ++i) {
			double dist;
			int new_group = findNearestCenter(samples.row(i), mu, invCovar, dist);
			num_members[new_group]++;

			if (new_group != groups[i]) {
				groups[i] = new_group;
			}
		}

		// クラスタ中心を更新する
		mu = Mat_<double>::zeros(num_clusters, dimensions);
		for (int i = 0; i < samples.rows; ++i) {
			for (int k = 0; k < dimensions; ++k) {
				mu(groups[i], k) += samples(i, k) / (double)num_members[groups[i]];
			}
		}
	}

	bool updated = true;
	int count = 0;
	for (int iter = 0; iter < max_iterations && updated; ++iter) {
		updated = false;

		// 各サンプルに最も近いクラスタを求める
		vector<int> num_members(num_clusters, 0);
		for (int i = 0; i < samples.rows; ++i) {
			double dist;
			int group_id = findNearestCenter(samples.row(i), mu, invCovar, dist);
			num_members[group_id]++;

			if (group_id != groups[i]) {
				groups[i] = group_id;
				updated = true;
			}
		}

		// クラスタ中心を更新する
		mu = Mat_<double>::zeros(num_clusters, dimensions);
		for (int i = 0; i < samples.rows; ++i) {
			for (int k = 0; k < dimensions; ++k) {
				mu(groups[i], k) += samples(i, k) / (double)num_members[groups[i]];
			}
		}
	}
}

/**
 * 与えられたサンプルに対して、ユークリッド距離を使って、最も近いクラスタ中心のIDを返却する。
 *
 * @param sample			サンプル
 * @param mu				クラスタ中心のリスト
 * @param min_dist [OUT]	最近傍クラスタへの距離
 * @return					最近傍のクラスタ中心のID
 */
/*
int KMeans::findNearestCenter(const Mat_<double>& sample, const Mat_<double>& mu, double& min_dist) {
	min_dist = std::numeric_limits<double>::max();

	int group_id = -1;

	for (int j = 0; j < mu.rows; ++j) {
		double d = norm(sample - mu.row(j));

		if (d < min_dist) {
			min_dist = d;
			group_id = j;
		}
	}

	return group_id;
}
*/

/**
 * 与えられたサンプルに対して、Mahalanobis距離を使って、最も近いクラスタ中心のIDを返却する。
 *
 * @param sample			サンプル
 * @param mu				クラスタ中心
 * @param invCovar			共分散行列の逆行列
 * @param min_dist [OUT]	最近傍クラスタ中心への距離
 * @return					最近傍のクラスタ中心のID
 */
int KMeans::findNearestCenter(const Mat_<double>& sample, const Mat_<double>& mu, const Mat& invCovar, double& min_dist) {
	min_dist = std::numeric_limits<double>::max();

	int group_id = -1;

	for (int j = 0; j < mu.rows; ++j) {
		double dist = cv::Mahalanobis(sample, mu.row(j), invCovar);

		if (dist < min_dist) {
			min_dist = dist;
			group_id = j;
		}
	}

	return group_id;
}

int KMeans::sampleFromCdf(std::vector<double> &cdf) {
	double rnd = (double)rand() / RAND_MAX * cdf.back();

	for (int i = 0; i < cdf.size(); ++i) {
		if (rnd <= cdf[i]) return i;
	}

	return cdf.size() - 1;
}

int KMeans::sampleFromPdf(std::vector<double> &pdf) {
	if (pdf.size() == 0) return 0;

	std::vector<double> cdf(pdf.size(), 0.0f);
	cdf[0] = pdf[0];
	for (int i = 1; i < pdf.size(); ++i) {
		if (pdf[i] >= 0) {
			cdf[i] = cdf[i - 1] + pdf[i];
		} else {
			cdf[i] = cdf[i - 1];
		}
	}

	return sampleFromCdf(cdf);
}