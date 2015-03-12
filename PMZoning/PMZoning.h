#pragma once

#include <vector>
#include <opencv/cv.h>
#include <opencv/highgui.h>

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
	static enum { TYPE_RESIDENTIAL = 0, TYPE_COMMERCIAL, TYPE_INDUSTRIAL, TYPE_PARK };
	int city_size;
	int grid_size;
	vector<float> zone_distribution;
	Mat_<uchar> zones;

public:
	PMZoning(int city_size, int grid_size, vector<float>& zone_distribution);
	~PMZoning();

	void update();
	void save(char* filename);

private:
	void computeMooreNeighborhood(int r, int c, vector<int>& neighbors);
	void initialZoning(vector<float>& zone_distribution);
};

