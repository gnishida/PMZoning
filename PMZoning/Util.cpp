#include "Util.h"
#include <random>

/**
 * Uniform乱数[0, 1)を生成する
 */
float Util::genRand() {
	return rand() / (float(RAND_MAX) + 1);
}

/**
 * 指定された範囲[a, b)のUniform乱数を生成する
 */
float Util::genRand(float a, float b) {
	return genRand() * (b - a) + a;
}

/**
 * Normal distributionを使用して乱数を生成する。
 */
float Util::genRandNormal(float mean, float variance) {
	/*
	static std::default_random_engine generator;

	std::normal_distribution<float> distribution(mean, sqrtf(variance));
	return distribution(generator);
	*/

#if 1
	float m = mean;
	float s = sqrt(variance);

	/* mean m, standard deviation s */
	float x1, x2, w, y1;
	static float y2;
	static int use_last = 0;

	if (use_last) {	/* use value from previous call */
		y1 = y2;
		use_last = 0;
	} else {
		do {
			x1 = 2.0 * genRand(0.0f, 1.0f) - 1.0;
			x2 = 2.0 * genRand(0.0f, 1.0f) - 1.0;
			w = x1 * x1 + x2 * x2;
		} while ( w >= 1.0 );

		w = sqrt( (-2.0 * log( w ) ) / w );
		y1 = x1 * w;
		y2 = x2 * w;
		use_last = 1;
	}

	return m + y1 * s;
#endif
}

