#include "Zoning.h"
#include "ModifiedBrushFire.h"
#include "GraphUtil.h"
#include <QFile>
#include <QTextStream>
#include "Util.h"

const int Zoning::NUM_TYPES = 4;
const int Zoning::NUM_COMPONENTS = 6;

Zoning::Zoning(int city_size, int grid_size, vector<float>& zone_distribution, RoadGraph& roads) {
	this->city_size = city_size;
	this->grid_size = grid_size;
	this->zone_distribution = zone_distribution;
	this->roads = roads;

	zones = Mat_<uchar>::zeros(grid_size, grid_size);

	computeAccessibility(0, properties[COM_MAJOR_ROADS]);
	computeAccessibility(1, properties[COM_MINOR_ROADS]);
}

Zoning& Zoning::operator=(const Zoning &ref) {
	city_size = ref.city_size;
	grid_size = ref.grid_size;
	ref.zones.copyTo(zones);
	GraphUtil::copyRoads(ref.roads, roads);
	copy(ref.zone_distribution.begin(), ref.zone_distribution.end(), zone_distribution.begin());
	for (int i = 0; i < NUM_COMPONENTS; ++i) {
		ref.properties[i].copyTo(properties[i]);
	}

	return *this;
}

/**
 * 各セルのpropertyベクトルを計算する。
 * 各コンポーネントは、
 *   - 住宅地への近さ
 *   - 商業地への近さ
 *   - 工業地への近さ
 *   - 公園への近さ
 *   - major道路への近さ
 *   - minor道路への近さ
 */
void Zoning::computePropertyVectors() {
	// 各propertyベクトル用の行列を初期化
	for (int k = 0; k < NUM_COMPONENTS; ++k) {
		properties[k] = Mat_<float>(grid_size, grid_size);
	}

	// 各要素ごとに、距離マップを計算
	Mat_<float> distMap[NUM_COMPONENTS];
	for (int k = 0; k < NUM_TYPES; ++k) {
		computeDistanceMap(k, distMap[k]);
	}
	computeAccessibility(0, distMap[NUM_TYPES]);
	computeAccessibility(1, distMap[NUM_TYPES + 1]);

	// 距離マップに基づいて、propertyベクトルを生成する
	for (int r = 0; r < grid_size; ++r) {
		for (int c = 0; c < grid_size; ++c) {
			for (int k = 0; k < NUM_COMPONENTS; ++k) {
				properties[k](r, c) = attenuation(distMap[k](r, c), 50);
			}
		}
	}
}

/**
 * スコアを計算する。
 *
 * @param preferences		ユーザのpreferenceベクトル (vector<重み、好みベクトル>)
 */
float Zoning::computeScore(vector<pair<float, vector<float> > >& preferences) {
	std::vector<std::vector<std::pair<float, Vec2i> > > all_scores(preferences.size());

	int num_cells = 0;

	// 各セルの、各ユーザによるスコアを計算
	for (int r = 0; r < grid_size; ++r) {
		for (int c = 0; c < grid_size; ++c) {
			if (zones(r, c) != TYPE_RESIDENTIAL) continue;

			num_cells++;

			for (int u = 0; u < preferences.size(); ++u) {
				float score = 0.0f;
				for (int k = 0; k < 6; ++k) {
					score += properties[k](r, c) * preferences[u].second[k];
				}

				all_scores[u].push_back(make_pair(score, Vec2i(c, r)));
			}
		}
	}

	// スコアでソートする
	for (int u = 0; u < preferences.size(); ++u) {
		std::sort(all_scores[u].begin(), all_scores[u].end(), GreaterScore);
	}

	// 占有率チェック用
	Mat_<float> occupied = Mat_<float>::zeros(grid_size, grid_size);

	// ポインタ
	vector<int> pointer(preferences.size(), 0);

	float score = 0.0f;
	int count = num_cells;
	int xxx = 0;
	while (count > 0) {
		for (int u = 0; u < preferences.size() && count > 0; ++u) {
			Vec2i cell = all_scores[u][pointer[u]].second;
			while (occupied(cell[1], cell[0]) >= 1) {
				pointer[u]++;
				cell = all_scores[u][pointer[u]].second;
			}
			
			occupied(cell[1], cell[0]) += preferences[u].first;
			score += all_scores[u][pointer[u]].first;

			if (occupied(cell[1], cell[0]) >= 1.0) count--;
		}
	}

	return score / num_cells;
}

/**
 * preferencesベクトルを、指定された数にグループ分けする。
 *
 * @param preferences		preferenceベクトルのリスト (２次元配列 [ユーザID][要素番号])
 * @param num				グループの数
 * @return					各グループの重みと、代表的なpreferenceベクトル（重みの合計は1となる)
 */
vector<pair<float, vector<float> > > Zoning::clusterPreferences(vector<vector<float> >& preferences, int num) {
	vector<pair<float, vector<float> > > groups;

	// まず、各コンポーネント毎の分散を計算し、それが1となるようnormalizeする
	// なお、この考え方は、「マハラノビス距離」と考えることもできる
	for (int k = 0; k < NUM_COMPONENTS; ++k) {
		for (int i = 0; i < preferences.size(); ++i) {

		}
	}

	// K-meansクラスタリングを実施
}

/**
 * ランダムにpreferenceベクトルをnum個作成する。
 *
 * @param num		個数
 * @return			生成されたpreferenceベクトルのリスト
 */
vector<vector<float> > Zoning::generateRandomPreferences(int num) {
	vector<vector<float> > templates(3, vector<float>(NUM_COMPONENTS));
	templates[0][0] = 0.2; templates[0][1] = 0.5; templates[0][2] = -1; templates[0][3] = 0.2; templates[0][4] = -0.5; templates[0][5] = 0.5;
	templates[1][0] = 0.0; templates[1][1] = 1.0; templates[1][2] = -1; templates[1][3] = 0.0; templates[1][4] = 0.0; templates[1][5] = 1.0;
	templates[2][0] = 1.0; templates[2][1] = 0.2; templates[2][2] = -1; templates[2][3] = 0.0; templates[2][4] = 0.0; templates[2][5] = 0.5;
	for (int u = 0; u < 3; ++u) {
		Util::normalize(templates[u]);
	}

	vector<vector<float> > preferences;
	for (int u = 0; u < num; ++u) {
		vector<float> preference(NUM_COMPONENTS, 0.0f);

		for (int t = 0; t < templates.size(); ++t) {
			float w = Util::genRand();

			for (int k = 0; k < NUM_COMPONENTS; ++k) {
				preference[k] += w * templates[t][k];
			}
		}

		Util::normalize(preference);
		preferences.push_back(preference);
	}

	return preferences;
}

/**
 * ゾーンを画像として保存する。
 */
void Zoning::save(char* filename, int img_size) {
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

	// 指定された画像サイズにリサイズする
	cv::resize(m, m, Size(img_size, img_size), INTER_NEAREST);

	// 道路を表示
	RoadEdgeIter ei, eend;
	for (boost::tie(ei, eend) = boost::edges(roads.graph); ei != eend; ++ei) {
		for (int i = 0; i < roads.graph[*ei]->polyline.size() - 1; ++i) {
			QVector2D p1(roads.graph[*ei]->polyline[i].x(), roads.graph[*ei]->polyline[i].y());
			QVector2D p2(roads.graph[*ei]->polyline[i+1].x(), roads.graph[*ei]->polyline[i+1].y());

			p1 = p1 / (float)city_size * img_size + QVector2D(img_size, img_size) * 0.5f;
			p2 = p2 / (float)city_size * img_size + QVector2D(img_size, img_size) * 0.5f;

			int lineWidth = 1;
			if (roads.graph[*ei]->type != RoadEdge::TYPE_STREET) {
				lineWidth = 2;
			}

			cv::line(m, Point(p1.x(), p1.y()), Point(p2.x(), p2.y()), Scalar(0, 160, 0), lineWidth);
		}
	}

	cv::flip(m, m, 0);
	cv::imwrite(filename, m);
}

bool Zoning::GreaterScore(const std::pair<float, Vec2i>& rLeft, const std::pair<float, Vec2i>& rRight) { return rLeft.first > rRight.first; }

/**
 * 道路までのaccessibilityを計算する。
 * dist = 直近のAvenue道路までの距離とした時、accessibility = 1 / (1 + dist)
 * 「Real and virtual uban design」のスライドにあった計算式を使っている。
 * ただし、majorなら、avenue、highwayのみを考慮、minorなら、local streetのみを考慮する。
 *
 * @param type					0 - major / 1 - minor
 * @param accessibility [OUT]	距離マップを格納して返却する
 */
void Zoning::computeAccessibility(int type, Mat_<float>& accessibility) {
	accessibility = Mat_<float>::zeros(grid_size, grid_size);

	// 距離係数（この距離離れると、accessibilityが半減するというイメージ）
	const float distanceFactor = 50.0f;

	int roadType;
	if (type == 0) {
		roadType = RoadEdge::TYPE_AVENUE | RoadEdge::TYPE_HIGHWAY;
	} else {
		roadType = RoadEdge::TYPE_STREET;
	}

	Mat_<uchar> data = Mat_<uchar>::zeros(grid_size, grid_size);
	RoadEdgeIter ei, eend;
	for (boost::tie(ei, eend) = boost::edges(roads.graph); ei != eend; ++ei) {
		if (roads.graph[*ei]->type & roadType) {
			Polyline2D polyline = GraphUtil::finerEdge(roads, *ei, 1.0f);
			for (int i = 0; i < polyline.size(); ++i) {
				QVector2D pt = cityToGrid(polyline[i]);
				if (pt.x() >= 0 && pt.x() < grid_size && pt.y() >= 0 && pt.y() < grid_size) {
					data(pt.y(), pt.x()) = 1;
				}
			}
		}
	}

	// Brushfireアルゴリズムで、距離マップを計算する
	modifiedbrushfire::ModifiedBrushFire bf(grid_size, grid_size, data);
	//int error = bf.check();
	//printf("Error: %d\n", error);

	// accessibilityを計算する
	for (int r = 0; r < grid_size; ++r) {
		for (int c = 0; c < grid_size; ++c) {
			float dist = bf.distMap()(r, c);

			// グリッドサイズを、実際の距離に変換する
			accessibility(r, c) = dist / (float)grid_size * city_size;
		}
	}

#if 0
	// 画像として保存しておく
	cv::Mat m;
	accessibility.convertTo(m, CV_8U, 255.0f);
	cv::flip(m, m, 0);

	if (type == 0) {
		cv::imwrite("majorAccessibility.jpg", m);
	} else {
		cv::imwrite("minorAccessibility.jpg", m);
	}
#endif
}

/**
 * 指定されたゾーンタイプに関する距離マップを計算する。
 *
 * @param type		ゾーンタイプ
 */
void Zoning::computeDistanceMap(int type, Mat_<float>& distMap) {
	distMap = Mat_<float>(grid_size, grid_size);

	// 距離マップを計算するために、指定されたゾーンタイプの場所を記録したマップを作成する
	Mat_<uchar> data = Mat_<uchar>::zeros(grid_size, grid_size);
	for (int r = 0; r < grid_size; ++r) {
		for (int c = 0; c < grid_size; ++c) {
			if (zones(r, c) == type) {
				data(r, c) = 1;
			}
		}
	}

	// Brushfireアルゴリズムで、距離マップを計算する
	modifiedbrushfire::ModifiedBrushFire bf(grid_size, grid_size, data);

	// グリッドサイズを、実際の距離に変換する
	for (int r = 0; r < grid_size; ++r) {
		for (int c = 0; c < grid_size; ++c) {
			float dist = bf.distMap()(r, c);

			// グリッドサイズを、実際の距離に変換する
			distMap(r, c) = dist / (float)grid_size * city_size;
		}
	}
}

/**
 * 減衰関数。1/(1+x/k)
 *
 * @param x			xの値
 * @param factor	減衰係数
 * @return			1/(1+x/k)
 */
float Zoning::attenuation(float x, float factor) {
	return 1.0 / (1.0 + x / factor);
}

QVector2D Zoning::gridToCity(const QVector2D& pt) {
	return (pt + QVector2D(0.5f, 0.5f)) / (float)grid_size * city_size - QVector2D(city_size, city_size) * 0.5f;
}

QVector2D Zoning::cityToGrid(const QVector2D& pt) {
	QVector2D ret = pt / (float)city_size * grid_size + QVector2D(grid_size, grid_size) * 0.5f;
	ret.setX((int)ret.x());
	ret.setY((int)ret.y());
	return ret;
}

