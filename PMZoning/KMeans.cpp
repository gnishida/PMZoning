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

	mu = Mat_<double>(samples.rows, dimensions);
	groups.resize(samples.rows, -1);

	// ランダムにサンプルを選び、クラスタ中心とする
	mu = Mat_<double>(num_clusters, dimensions);
	for (int j = 0; j < num_clusters; ++j) {
		int s = (double)rand() / RAND_MAX * samples.rows;

		Mat_<double> temp = mu.rowRange(j, j + 1);
		samples.row(s).copyTo(temp);
	}

	// 初期クラスタリング（最初の１回だけ、Euclidian距離を使用してクラスタリング）
	{
		// 各サンプルに最も近いクラスタを求める
		vector<int> num_members(num_clusters, 0);
		for (int i = 0; i < samples.rows; ++i) {
			double min_dist = std::numeric_limits<double>::max();

			int new_group = -1;

			for (int j = 0; j < num_clusters; ++j) {
				double dist = norm(samples.row(i) - mu.row(j));
				if (dist < min_dist) {
					min_dist = dist;
					new_group = j;
				}
			}
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

	// サンプルの共分散行列を計算する
	Mat covar, mean;
	calcCovarMatrix(samples, covar, mean, CV_COVAR_NORMAL | CV_COVAR_ROWS);
	covar = covar / (samples.rows - 1);

	// 共分散行列の逆行列を計算する
	Mat invCovar;
	cv::invert(covar, invCovar, DECOMP_SVD);

	bool updated = true;
	int count = 0;
	for (int iter = 0; iter < max_iterations && updated; ++iter) {
		updated = false;

		// 各サンプルに最も近いクラスタを求める
		vector<int> num_members(num_clusters, 0);
		for (int i = 0; i < samples.rows; ++i) {
			double min_dist = std::numeric_limits<double>::max();

			int new_group = -1;

			for (int j = 0; j < num_clusters; ++j) {
				double dist = cv::Mahalanobis(samples.row(i), mu.row(j), invCovar);
				if (dist < min_dist) {
					min_dist = dist;
					new_group = j;
				}
			}
			num_members[new_group]++;

			if (new_group != groups[i]) {
				groups[i] = new_group;
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

