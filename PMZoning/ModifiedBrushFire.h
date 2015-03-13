#pragma once

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <list>

using namespace cv;
using namespace std;

namespace modifiedbrushfire {

class ModifiedBrushFire {
private:
	int width;				// グリッドの横のセルの数
	int height;				// グリッドの縦のセルの数
	Mat_<uchar> data;		// 1 - ストア / 0 - 無し
	Mat_<float> dist;		// 距離マップ
	Mat_<Vec2i> obst;		// 直近のストアの座標
	Mat_<bool> toRaise;		// 要更新マーク
	list<Vec2i> queue;

public:
	ModifiedBrushFire(int width, int height, Mat& data);
	
	const Mat_<float>& distMap() { return dist; }

	void updateDistanceMap();
	void setStore(int r, int c);
	void removeStore(int r, int c);
	int check();

private:
	void init();
	bool isOcc(const Vec2i& pt);
	float distance(int r1, int c1, int r2, int c2);
	void clearCell(int r, int c);
	void raise(int r, int c);
	void lower(int r, int c);
};

}