#pragma once

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "RoadGraph.h"

using namespace std;
using namespace cv;

class Zoning {
protected:
	static enum { TYPE_RESIDENTIAL = 0, TYPE_COMMERCIAL = 1, TYPE_INDUSTRIAL = 2, TYPE_PARK = 3, TYPE_UNUSED = 9 };
	static enum { COM_RESIDENTIAL = 0, COM_COMMERCIAL = 1, COM_INDUSTRIAL = 2, COM_PARK = 3, COM_MAJOR_ROADS = 4, COM_MINOR_ROADS = 5 };

	/** ゾーンタイプの種類の数　*/
	static const int NUM_TYPES;

	/** propertyベクトルの要素数の数 */
	static const int NUM_COMPONENTS;

	int city_size;
	int grid_size;
	Mat_<uchar> zones;
	RoadGraph roads;
	vector<float> zone_distribution;
	Mat_<float> properties[6];

public:
	Zoning(int city_size, int grid_size, vector<float>& zone_distribution, RoadGraph& roads);
	Zoning& operator=(const Zoning &ref);

	Mat_<uchar> zoneMap() { return zones.clone(); }
	void computePropertyVectors();
	float computeScore(vector<pair<float, vector<float> > >& preferences);
	static vector<pair<float, vector<float> > > clusterPreferences(vector<vector<float> >& preferences, int num);
	static vector<vector<float> > generateRandomPreferences(int num);
	void save(char* filename, int img_size);

protected:
	static bool GreaterScore(const std::pair<float, Vec2i>& rLeft, const std::pair<float, Vec2i>& rRight);
	void computeAccessibility(int type, Mat_<float>& distMap);
	void computeDistanceMap(int type, Mat_<float>& distMap);
	float attenuation(float x, float factor);
	QVector2D gridToCity(const QVector2D& pt);
	QVector2D cityToGrid(const QVector2D& pt);
};

