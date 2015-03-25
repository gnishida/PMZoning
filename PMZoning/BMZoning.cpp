#include "BMZoning.h"
#include "GraphUtil.h"
#include "Util.h"

BMZoning::BMZoning(int city_size, int grid_size, vector<float>& zone_distribution, RoadGraph& roads) : Zoning(city_size, grid_size, zone_distribution, roads) {
	// ゾーンをランダムに決定する
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

	// 各セルの道路の長さを計算する
	Mat_<float> r = Mat_<float>::zeros(grid_size, grid_size);
	RoadEdgeIter ei, eend;
	for (boost::tie(ei, eend) = boost::edges(roads.graph); ei != eend; ++ei) {
		Polyline2D polyline = GraphUtil::finerEdge(roads, *ei, 10.0f);

		for (int i = 0; i < polyline.size() - 1; ++i) {
			QVector2D pt = cityToGrid(polyline[i]);
			r(pt.y(), pt.x()) += (polyline[i + 1] - polyline[i]).length();
		}
	}

	// 各セルのキャパシティに基づき、人口・仕事を配分する
	for (int i = 0; i < 3; ++i) {
		people[i] = Mat_<int>::zeros(grid_size, grid_size);
	}
	for (int i = 0; i < grid_size; ++i) {
		for (int j = 0; j < grid_size; ++j) {
			if (zones(j, i) == TYPE_RESIDENTIAL) {
				people[0](j, i) = r(j, i) / 10.0;
			} else if (zones(j, i) == TYPE_COMMERCIAL) {
				people[1](j, i) = r(j, i) / 10.0f;
			} else if (zones(j, i) == TYPE_INDUSTRIAL) {
				people[2](j, i) = r(j, i) / 10.0f;
			}
		}
	}
}

void BMZoning::update() {
	// 余剰分の人口を計算
	Mat_<int> sum_people[3];
	int total_people[3];
	for (int i = 0; i < 3; ++i) {
		Mat_<int> sum_people;
		reduce(people[i], sum_people, 1, CV_REDUCE_SUM);
		reduce(sum_people, sum_people, 0, CV_REDUCE_SUM);
		total_people[i] = sum_people(0, 0);
	}

	// 余剰分の人口をセルから削除
	int d_people[3];
	d_people[0] = (total_people[1] + total_people[2] - total_people[0]) * 0.1;
	d_people[1] = -(float)total_people[0] / (total_people[1] + total_people[2]) * d_people[0];
	d_people[2] = -(float)total_people[0] / (total_people[1] + total_people[2]) * d_people[0];
	for (int i = 0; i < 3; ++i) {
		if (d_people[i] < 0) {
			removePeople(i, -d_people[i]);
			total_people[i] += d_people[i];
		}
	}

	// 人口の10%をセルから削除
	for (int i = 0; i < 3; ++i) {
		removePeople(i, total_people[i] * 0.1);
	}

	// 人口の10%をセルに追加
	for (int i = 0; i < 3; ++i) {
		if (d_people[i] > 0) {
			addPeople(i, total_people[i] * 0.1 + d_people[i]);
		} else {
			addPeople(i, total_people[i] * 0.1);
		}
	}
}

/**
 * 各セルのpropertyベクトルを計算する。
 */
void BMZoning::computeProperties() {
	// preferenceベクトル
	vector<float> w_q(9); w_q[0] = 0.1; w_q[1] = 0.1; w_q[2] = -1; w_q[3] = 0.01; w_q[4] = 0.2; w_q[5] = 0; w_q[6] = 0; w_q[7] = -0.3; w_q[8] = 0;
	vector<float> w_l(6); w_l[0] = 0.1; w_l[1] = 0.1; w_l[2] = -0.1; w_l[3] = 0.01; w_l[4] = 0.2; w_l[5] = 0;;
	vector<float> w_m(6); w_m[0] = -1; w_m[1] = 0.1; w_m[2] = 0.1; w_m[3] = -0.01; w_m[4] = 0.2; w_m[5] = -0.1;

	for (int i = 0; i < 9; ++i) {
		properties[i] = Mat_<float>::zeros(grid_size, grid_size);
	}
	for (int r = 0; r < grid_size; ++r) {
		for (int c = 0; c < grid_size; ++c) {
			vector<float> prop(9);
			prop[0] = computeProximity(0, c, r, 2);
			prop[1] = computeProximity(1, c, r, 2);
			prop[2] = computeProximity(2, c, r, 2);
			prop[3] = computeProximity(3, c, r, 2);
			prop[4] = computeAccessibility(c, r, 2);
			prop[5] = 0.0;
			prop[6] = 0.0;
			prop[7] = Util::dot(w_l, prop);	// 地価
			prop[8] = Util::dot(w_m, prop);	// アクセシビリティ
			prop[6] = Util::dot(w_q, prop);	// 質

			for (int k = 0; k < 9; ++k) {
				properties[k](r, c) = prop[k];
			}
		}
	}
}

/**
 * 指定されたセルを中心とする一定範囲の中で、指定されたゾーンタイプ÷そこまでの距離
 * の和を返却する。
 */
float BMZoning::computeProximity(int type, int x, int y, int window_size) {
	float total = 0.0f;
	float cell_length = (float)city_size / grid_size;

	for (int dx = -window_size; dx <= window_size; ++dx) {
		if (x + dx < 0 || x + dx >= grid_size) continue;
		for (int dy = -window_size; dy <= window_size; ++dy) {
			if (y + dy < 0 || y + dy >= grid_size) continue;
			if (zones(y + dy, x + dx) != type) continue;

			total += 1.0 / (1.0 + sqrtf(dx * dx + dy * dy));
		}
	}

	return total;
}

/**
 * 指定されたセルを中心とする一定範囲の中で、道路の交差点÷そこまでの距離の和を返却する。
 */
float BMZoning::computeAccessibility(int x, int y, int window_size) {
	float total = 0.0f;

	float cell_length = (float)city_size / grid_size;
	float dist_max = SQR(window_size * cell_length);

	QVector2D pt = gridToCity(QVector2D(x, y));

	RoadVertexIter vi, vend;
	for (boost::tie(vi, vend) = boost::vertices(roads.graph); vi != vend; ++vi) {
		float d = (roads.graph[*vi]->pt - pt).lengthSquared();
		if (d < dist_max) {
			total += 1.0 / (1.0 + sqrtf(d));
		}
	}

	return total;
}

/**
 * 指定された人数を、セルから削除する。セルはランダムに選択し、1人だけ削除し、これを繰り返す。
 */
void BMZoning::removePeople(int type, int num) {
	while (num > 0) {
		int cell_id = Util::genRand(0, grid_size * grid_size);
		int x = cell_id % grid_size;
		int y = cell_id / grid_size;

		if (people[type](y, x) > 0) {
			people[type](y, x)--;
			num--;
		}
	}
}

/**
 * 指定された人数を、セルに追加する。
 * セルは、人ならquality、商業ならland value、工業ならaccessibilityに基づいて決める。
 */
void BMZoning::addPeople(int type, int num) {
	while (num > 0) {
		vector<float> pdf;
		vector<int> cell_ids;
		for (int i = 0; i < 10; ++i) {
			int cell_id = Util::genRand(0, grid_size * grid_size);
			cell_ids.push_back(cell_id);
			int r = cell_id / grid_size;
			int c = cell_id % grid_size;
			pdf.push_back(properties[type + 6](r, c));
		}

		int index = Util::sampleFromPdf(pdf);
		int cell_id = cell_ids[index];
		int r = cell_id / grid_size;
		int c = cell_id % grid_size;

		people[type](r, c)++;
		num--;
	}
}
