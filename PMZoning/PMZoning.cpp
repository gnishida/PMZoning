#include "PMZoning.h"
#include "Util.h"

#define NUM_COMPONENTS	4

PMZoning::PMZoning(int city_size, int grid_size, vector<float>& zone_distribution, RoadGraph& roads) {
	this->city_size = city_size;
	this->grid_size = grid_size;
	this->zone_distribution = zone_distribution;
	this->roads = roads;

	zones = Mat_<uchar>::zeros(grid_size, grid_size);
	initialZoning(zone_distribution);
}

PMZoning::~PMZoning() {
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
	vector<int> histogram(NUM_COMPONENTS, 0);

	vector<float> needs(NUM_COMPONENTS);
	for (int i = 0; i < NUM_COMPONENTS; ++i) {
		needs[i] = 0;
	}

	for (int r = 0; r < grid_size; ++r) {
		for (int c = 0; c < grid_size; ++c) {
			if (zones(r, c) == TYPE_UNUSED) continue;

			int oldType = zones(r, c);

			// 隣接８個のセルの、各タイプごとの比率を計算
			vector<int> neighbors;
			computeMooreNeighborhood(r, c, neighbors);

			if (neighbors[TYPE_INDUSTRIAL] >= Util::genRand(1, 3) + 4.0 / (1.0f + expf(needs[TYPE_INDUSTRIAL]))) {
				zones(r, c) = TYPE_INDUSTRIAL;
			} else if (neighbors[TYPE_COMMERCIAL] >= Util::genRand(1, 3) + 4.0 / (1.0f + expf(needs[TYPE_COMMERCIAL]))) {
				zones(r, c) = TYPE_COMMERCIAL;
			} else if (neighbors[TYPE_PARK] >= Util::genRand(1, 3) + 4.0 / (1.0f + expf(needs[TYPE_PARK]))) {
				zones(r, c) = TYPE_PARK;
			} else {
				zones(r, c) = TYPE_RESIDENTIAL;
			}

			needs[oldType]++;
			needs[zones(r, c)]--;
		}
	}

	// distributionに合わせる

}

/**
 * ゾーンを画像として保存する。
 */
void PMZoning::save(char* filename) {
	Mat m(grid_size, grid_size, CV_8UC3);

	// ゾーンを表示
	for (int r = 0; r < grid_size; ++r) {
		for (int c = 0; c < grid_size; ++c) {
			cv::Vec3b p;
			if (zones(r, c) == TYPE_RESIDENTIAL) {			// 住宅街（黄色）
				p = cv::Vec3b(115, 255, 255);
			} else if (zones(r, c) == TYPE_COMMERCIAL) {	// 商業地（赤色）
				p = cv::Vec3b(0, 0, 255);
			} else if (zones(r, c) == TYPE_INDUSTRIAL) {	// 工業地（青色）
				p = cv::Vec3b(255, 0, 0);
			} else if (zones(r, c) == TYPE_PARK) {			// 公園（緑色）
				p = cv::Vec3b(85, 255, 0);
			} else if (zones(r, c) == TYPE_UNUSED) {		// 使用不可（白色）
				p = cv::Vec3b(255, 255, 255);
			}
			m.at<cv::Vec3b>(r, c) = p;
		}
	}

	// 道路を表示
	RoadEdgeIter ei, eend;
	for (boost::tie(ei, eend) = boost::edges(roads.graph); ei != eend; ++ei) {
		for (int i = 0; i < roads.graph[*ei]->polyline.size() - 1; ++i) {
			QVector2D p1(roads.graph[*ei]->polyline[i].x(), roads.graph[*ei]->polyline[i].y());
			QVector2D p2(roads.graph[*ei]->polyline[i+1].x(), roads.graph[*ei]->polyline[i+1].y());
			p1 = p1 * (float)grid_size / city_size + QVector2D(grid_size, grid_size) * 0.5f;
			p2 = p2 * (float)grid_size / city_size + QVector2D(grid_size, grid_size) * 0.5f;
			cv::line(m, Point(p1.x(), p1.y()), Point(p2.x(), p2.y()), Scalar(0, 160, 0), 1);
		}
	}

	cv::flip(m, m, 0);
	cv::imwrite(filename, m);
}

/**
 * 隣接８個のセルのゾーンタイプのヒストグラムを生成する。
 *
 * @param r			現在セルのY座標
 * @param c			現在セルのX座標
 * @param neighbors	隣接８個のセルのゾーンタイプのヒストグラム
 */
void PMZoning::computeMooreNeighborhood(int r, int c, vector<int>& neighbors) {
	neighbors.resize(NUM_COMPONENTS, 0);

	for (int rr = r - 1; rr <= r + 1; ++rr) {
		if (rr < 0 || rr >= grid_size) continue;
		for (int cc = c - 1; cc <= c + 1; ++cc) {
			if (cc < 0 || cc >= grid_size) continue;
			if (rr == r && cc == c) continue;
			
			if (zones(rr, cc) < NUM_COMPONENTS) {
				neighbors[zones(rr, cc)]++;
			}
		}
	}
}

/**
 * 指定された配分率に基づき、ランダムに初期ゾーンを決定する。
 *
 * @param zone_distribution		ゾーンタイプの配分率
 */
void PMZoning::initialZoning(vector<float>& zone_distribution) {
	assert(zone_distribution.size() == NUM_COMPONENTS);

	vector<float> expectedNums(NUM_COMPONENTS);
	for (int i = 0; i < NUM_COMPONENTS; ++i) {
		expectedNums[i] = grid_size * grid_size * zone_distribution[i];
	}

	for (int r = 0; r < grid_size; ++r) {
		for (int c = 0; c < grid_size; ++c) {
			unsigned char type = Util::sampleFromPdf(expectedNums);
			zones(r, c) = type;
			expectedNums[type]--;
		}
	}
}
