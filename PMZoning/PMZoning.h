#pragma once

#include <vector>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "RoadGraph.h"

using namespace std;
using namespace cv;

/**
 * Procedural modeling to generate zoning
 *
 * Zone type: 0 residential
 *            1 commercial
 *            2 industrial
 *            3 park
 */
class PMZoning {
private:
	static enum { TYPE_RESIDENTIAL = 0, TYPE_COMMERCIAL, TYPE_INDUSTRIAL, TYPE_PARK, TYPE_UNUSED };
	int city_size;
	int grid_size;
	vector<float> zone_distribution;
	Mat_<uchar> zones;
	RoadGraph roads;
	Mat_<float> accessibility;

public:
	PMZoning(int city_size, int grid_size, vector<float>& zone_distribution, RoadGraph& roads);
	~PMZoning();

	void update();
	void save(char* filename, int img_size);

private:
	void computeMooreNeighborhood(int r, int c, vector<int>& neighbors);
	void computeAccessibility();
	void initialZoning(vector<float>& zone_distribution);
	QVector2D gridToCity(const QVector2D& pt);
	QVector2D cityToGrid(const QVector2D& pt);
};

