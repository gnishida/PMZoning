#include "ModifiedBrushFire.h"
#include <limits>

namespace modifiedbrushfire {

const float MAX_DIST = numeric_limits<float>::max();
const Vec2i UNDEFINED = Vec2i(-1, -1);

ModifiedBrushFire::ModifiedBrushFire(int width, int height, Mat& data) {
	this->width = width;
	this->height = height;
	data.copyTo(this->data);
	init();
}

/**
 * 現在のキューに基づいて、距離マップを更新する。
 */
void ModifiedBrushFire::updateDistanceMap() {
	while (!queue.empty()) {
		Vec2i s = queue.front();
		queue.pop_front();

		if (toRaise(s[0], s[1])) {
			raise(s[0], s[1]);
		} else if (isOcc(obst(s[0], s[1]))) {
			lower(s[0], s[1]);
		}
	}
}

/**
 * 指定された座標に店を配置し、キューを更新する。
 * 実際に距離マップを更新するには、updateDistanceMapを呼び出すこと。
 *
 * @param r		Y座標
 * @param c		X座標
 */
void ModifiedBrushFire::setStore(int r, int c) {
	data(r, c) = 1;
	obst(r, c) = Vec2i(r, c);
	dist(r, c) = 0.0f;

	queue.push_back(Vec2i(r, c));
}

/**
 * 指定された座標のセルの店を削除する。
 *
 * @param s			セルのインデックス番号
 * @param featureId	店の種類
 */
void ModifiedBrushFire::removeStore(int r, int c) {
	// もし店がないなら、何もしない
	if (data(r, c) == 0) return;

	clearCell(r, c);

	toRaise(r, c) = true;
	data(r, c) = 0;

	queue.push_back(Vec2i(r, c));
}

/**
 * 計算したdistance mapが正しいか、チェックする。
 */
int ModifiedBrushFire::check() {
	int count = 0;

	for (int r = 0; r < height; ++r) {
		for (int c = 0; c < width; ++c) {
			float min_dist = MAX_DIST;

			for (int r2 = 0; r2 < height; ++r2) {
				for (int c2 = 0; c2 < width; ++c2) {
					if (data(r2, c2) == 1) {
						float d = distance(r2, c2, r, c);
						if (d < min_dist) {
							min_dist = d;
						}
					}
				}
			}

			if (dist(r, c) != min_dist) {
				count++;
			}
		}
	}
	
	return count;
}

/**
 * 初期化する
 */
void ModifiedBrushFire::init() {
	queue.clear();

	dist = Mat_<float>(height, width);
	obst = Mat_<Vec2i>(height, width);
	toRaise = Mat_<bool>(height, width);

	for (int r = 0; r < height; ++r) {
		for (int c = 0; c < width; ++c) {
			toRaise(r, c) = false;
			if (data(r, c) == 1) {
				setStore(r, c);
			} else {
				dist(r, c) = MAX_DIST;
				obst(r, c) = UNDEFINED;
			}
		}
	}

	updateDistanceMap();
}

/**
 * もし、指定された座標の直近の店が、指定された座標そのものなら、trueを返却する。
 * (要するに、その座標が店ならtrueを返却するということ。なぜ、data(r, c)==1でチェックしないの？)
 *
 * @param r		Y座標
 * @param c		X座標
 * @return		true / false
 */
bool ModifiedBrushFire::isOcc(const Vec2i& pt) {
	return obst(pt[0], pt[1]) == pt;
}

/** 
 * 2つのセル間の距離を返却する。
 *
 * @param r1		1つ目のセルのY座標
 * @param c1		1つ目のセルのX座標
 * @param r2		2つ目のセルのY座標
 * @param c2		2つ目のセルのX座標
 * @return			2つのセル間の距離
 */
float ModifiedBrushFire::distance(int r1, int c1, int r2, int c2) {
	return sqrtf((r1 - r2) * (r1 - r2) + (c1 - c2) * (c1 - c2));
}

/**
 * 指定されたセルの状態を初期化する
 */
void ModifiedBrushFire::clearCell(int r, int c) {
	dist(r, c) = MAX_DIST;
	obst(r, c) = UNDEFINED;
}

/**
 * 周辺のセルに、「要更新」マークを伝播させる。また、伝播された周囲のセルをqueueへ格納する。
 *
 * @param r		Y座標
 * @param c		X座標
 */
void ModifiedBrushFire::raise(int r, int c) {
	for (int rr = r - 1; rr <= r + 1; ++rr) {
		for (int cc = c - 1; cc <= c + 1; ++cc) {
			// 自分自身ならスキップ
			if (rr == r && cc == c) continue;

			// グリッドの外ならスキップ
			if (rr < 0 || rr >= height || cc < 0 || cc >= width) continue;

			// 既に要更新フラグがONなら、スキップ
			if (toRaise(rr, cc)) continue;

			// 既に直近の店の座標ががクリア済みなら、スキップ
			if (obst(rr, cc) == UNDEFINED) continue;

			// 直近の店が、削除されていたら、要更新だ！
			if (!isOcc(obst(rr, cc))) {
				clearCell(rr, cc);
				toRaise(rr, cc) = true;
			}

			queue.push_back(Vec2i(rr, cc));
		}
	}

	// 要更新フラグは、最前線のセルだけフラグをONにする。
	// 従って、周囲のセルへ伝播し終わったら、当該セルの要更新フラグはOFFに戻す
	toRaise(r, c) = false;
}

/**
 * 周囲のセルの距離マップを更新していく。
 *
 * @param r		Y座標
 * @param c		X座標
 */
void ModifiedBrushFire::lower(int r, int c) {
	for (int rr = r - 1; rr <= r + 1; ++rr) {
		for (int cc = c - 1; cc <= c + 1; ++cc) {
			// 自分自身ならスキップ
			if (rr == r && cc == c) continue;

			// グリッドの外ならスキップ
			if (rr < 0 || rr >= height || cc < 0 || cc >= width) continue;

			if (toRaise(rr, cc)) continue;

			float d = distance(obst(r, c)[0], obst(r, c)[1], rr, cc);
			if (d < dist(rr, cc)) {
				dist(rr, cc) = d;
				obst(rr, cc) = obst(r, c);
				queue.push_back(Vec2i(rr, cc));
			}
		}
	}
}

}