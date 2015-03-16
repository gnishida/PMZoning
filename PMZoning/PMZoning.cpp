#include "PMZoning.h"
#include "Util.h"
#include "GraphUtil.h"
#include "RoadEdge.h"
#include "ModifiedBrushFire.h"
#include <QFile>

PMZoning::PMZoning(int city_size, int grid_size, vector<float>& zone_distribution, RoadGraph& roads) : Zoning(city_size, grid_size, zone_distribution, roads) {
}

/**
 * 指定された配分率に基づき、ランダムに初期ゾーンを決定する。
 *
 * @param zone_distribution		ゾーンタイプの配分率
 */
void PMZoning::initialZoning(vector<float>& zone_distribution) {
	assert(zone_distribution.size() == NUM_TYPES);

	vector<float> expectedNums(NUM_TYPES);
	for (int i = 0; i < NUM_TYPES; ++i) {
		expectedNums[i] = grid_size * grid_size * zone_distribution[i];
	}

	for (int r = 0; r < grid_size; ++r) {
		for (int c = 0; c < grid_size; ++c) {
			unsigned char type = Util::sampleFromPdf(expectedNums);
			zones(r, c) = type;
			expectedNums[type]--;
		}
	}

	// ニーズを初期化
	needs.resize(NUM_TYPES, 0);
}

/**
 * 何らかのルールに従い、現在のゾーンをよりリーズナブルなものに変更する。
 * とりあえず、セルオートマトンのアルゴリズムで更新してみよう。
 * つまり、隣接８個のセルの状態に基づいて、確率的に変更する。
 * 4^8=65536通りの状態があるよね。
 * 
 * 
 */
void PMZoning::update() {
	Mat_<uchar> new_zones(zones.size());

	for (int r = 0; r < grid_size; ++r) {
		for (int c = 0; c < grid_size; ++c) {
			if (zones(r, c) == TYPE_UNUSED) continue;
						
			// 隣接８個のセルの、各タイプごとの比率を計算
			vector<float> neighbors;
			computeMooreNeighborhood(r, c, neighbors, true);		

#if 0
			// probabilistic
			vector<float> prob(NUM_TYPES);
			prob[0] = neighbors[TYPE_RESIDENTIAL] * 1.6f + 1.0f / (1.0f + expf(-needs[TYPE_RESIDENTIAL])) - 0.5f;
			prob[1] = neighbors[TYPE_COMMERCIAL] * 0.9f + accessibility(r, c) * 0.7f + 1.0f / (1.0f + expf(-needs[TYPE_COMMERCIAL])) - 0.5f;
			prob[2] = neighbors[TYPE_INDUSTRIAL] * 1.2f + accessibility(r, c) * 0.4f + 1.0f / (1.0f + expf(-needs[TYPE_INDUSTRIAL])) - 0.5f;
			prob[3] = neighbors[TYPE_PARK] * 1.6f + 1.0 / (1.0f + expf(-needs[TYPE_PARK])) - 0.5f;

			prob[1] = max(0.0f, prob[1]);
			prob[2] = max(0.0f, prob[2]);
			prob[3] = max(0.0f, prob[3]);
			prob[0] = 1.0f - prob[1] - prob[2] - prob[3];

			new_zones(r, c) = Util::sampleFromPdf(prob);
			
#endif

#if 1
			// simple + accessibility
			if (neighbors[TYPE_COMMERCIAL] * 1.6f + attenuation(properties[COM_MAJOR_ROADS](r, c), 50) * 0.4f + 0.8f / (1.0f + expf(-needs[TYPE_COMMERCIAL])) - 1.0f >= Util::genRand(0, 1)) {
				new_zones(r, c) = TYPE_COMMERCIAL;
			} else if (neighbors[TYPE_INDUSTRIAL] * 1.6f + attenuation(properties[COM_MAJOR_ROADS](r, c), 50) * 0.4f + 0.8f / (1.0f + expf(-needs[TYPE_INDUSTRIAL])) - 1.0f >= Util::genRand(0, 1)) {
				new_zones(r, c) = TYPE_INDUSTRIAL;
			} else if (neighbors[TYPE_RESIDENTIAL] * 1.2f + attenuation(properties[COM_MAJOR_ROADS](r, c), 50) * 0.4f + 0.8f / (1.0f + expf(-needs[TYPE_RESIDENTIAL])) - 1.0f >= Util::genRand(0, 1)) {\
				new_zones(r, c) = TYPE_RESIDENTIAL;
			} else {
				new_zones(r, c) = TYPE_PARK;
			}
#endif

#if 0
			// simple update
			if (neighbors[TYPE_COMMERCIAL] >= Util::genRand(1, 3) + 4.0 / (1.0f + expf(needs[TYPE_COMMERCIAL]))) {
				new_zones(r, c) = TYPE_COMMERCIAL;
			} else if (neighbors[TYPE_INDUSTRIAL] >= Util::genRand(1, 3) + 4.0 / (1.0f + expf(needs[TYPE_INDUSTRIAL]))) {
				new_zones(r, c) = TYPE_INDUSTRIAL;
			} else if (neighbors[TYPE_PARK] >= Util::genRand(1, 3) + 4.0 / (1.0f + expf(needs[TYPE_PARK]))) {
				new_zones(r, c) = TYPE_PARK;
			} else {
				new_zones(r, c) = TYPE_RESIDENTIAL;
			}
#endif

			// ゾーンタイプのニーズを更新
			needs[zones(r, c)]++;		// このセルから削除されたゾーンタイプのニーズは増加する
			needs[new_zones(r, c)]--;	// このセルに使用されたゾーンタイプのニーズは減る
		}
	}
	
	new_zones.copyTo(zones);
}

/**
 * 隣接８個のセルのゾーンタイプのヒストグラムを生成する。
 *
 * @param r			現在セルのY座標
 * @param c			現在セルのX座標
 * @param neighbors	隣接８個のセルのゾーンタイプのヒストグラム
 */
void PMZoning::computeMooreNeighborhood(int r, int c, vector<float>& neighbors, bool normalize) {
	neighbors.resize(NUM_TYPES, 0);

	int sum = 0;
	for (int rr = r - 1; rr <= r + 1; ++rr) {
		if (rr < 0 || rr >= grid_size) continue;
		for (int cc = c - 1; cc <= c + 1; ++cc) {
			if (cc < 0 || cc >= grid_size) continue;
			if (rr == r && cc == c) continue;
			
			if (zones(rr, cc) < NUM_TYPES) {
				neighbors[zones(rr, cc)]++;
				sum++;
			}
		}
	}

	if (normalize) {
		for (int i = 0; i < neighbors.size(); ++i) {
			neighbors[i] /= (float)sum;
		}
	}
}

/**
 * Logistic関数みたいな関数
 * x=0の時の値がhになるようにする。
 */
float PMZoning::modifiedLogistic(float x, float h) {
	if (x >= 0.0f) {
		return 2.0f * (1.0f - h) / (1.0f + expf(-x)) - 1 + h * 2.0f;
	} else {
		return 2.0f * h / (1.0f + exp(-x));
	}
}
