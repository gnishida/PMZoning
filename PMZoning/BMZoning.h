#pragma once

#include "Zoning.h"

using namespace std;
using namespace cv;

/**
 * CarlosのBehavioral Modeling論文を参考に、simulationベースで
 * zoningを生成する。
 *
 * @author	Gen Nishida
 * @date	3/25/2015
 * @version	1.0
 */
class BMZoning : public Zoning {
private:
	Mat_<int> people[3];
	Mat_<float> properties[9];

public:
	BMZoning(int city_size, int grid_size, vector<float>& zone_distribution, RoadGraph& roads);

	void update();

private:
	void computeProperties();
	float computeProximity(int type, int x, int y, int window_size);
	float computeAccessibility(int x, int y, int window_size);
	void removePeople(int type, int num);
	void addPeople(int type, int num);
};

