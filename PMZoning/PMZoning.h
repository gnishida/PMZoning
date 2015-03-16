#pragma once

#include "Zoning.h"
#include <vector>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "RoadGraph.h"
#include <QMap>

using namespace std;
using namespace cv;

/**
 * Procedural modeling to generate zoning
 */
class PMZoning : public Zoning {
private:
	vector<float> needs;

public:
	PMZoning(int city_size, int grid_size, vector<float>& zone_distribution, RoadGraph& roads);

	void initialZoning(vector<float>& zone_distribution);
	void update();

private:
	void computeMooreNeighborhood(int r, int c, vector<float>& neighbors, bool normalize);
	float modifiedLogistic(float x, float h);
};

