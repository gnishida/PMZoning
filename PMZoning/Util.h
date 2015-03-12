#pragma once

#include <vector>

class Util {
protected:
	Util();

public:
	// random
	static float genRand();
	static float genRand(float a, float b);
	static float genRandNormal(float mean, float variance);

	template<typename T>
	static int sampleFromCdf(const std::vector<T> &cdf) {
		float rnd = genRand(0, cdf.back());

		for (int i = 0; i < cdf.size(); ++i) {
			if (rnd <= cdf[i]) return i;
		}

		return cdf.size() - 1;
	}

	template<typename T>
	static int sampleFromPdf(const std::vector<T> &pdf) {
		if (pdf.size() == 0) return 0;

		std::vector<T> cdf(pdf.size(), 0.0f);
		cdf[0] = pdf[0];
		for (int i = 1; i < pdf.size(); ++i) {
			if (pdf[i] >= 0) {
				cdf[i] = cdf[i - 1] + pdf[i];
			} else {
				cdf[i] = cdf[i - 1];
			}
		}

		return sampleFromCdf(cdf);
	}

	/**
	 * dot productを計算する。
	 * わざと、v2の方がサイズが大きくても許容している。
	 * v2のサイズが大きい場合は、v1のサイズの分までで、dot productを計算する。
	 */
	template<typename T>
	static float dot(std::vector<T> v1, std::vector<T> v2) {
		float ret = 0.0f;

		assert(v1.size() <= v2.size());

		for (int i = 0; i < v1.size(); ++i) {
			ret += v1[i] * v2[i];
		}

		return ret;
	}

	template<typename T>
	static void normalize(std::vector<T>& v) {
		T len = dot(v, v);

		if (len == 0) return;
		len = sqrt(len);

		for (int i = 0; i < v.size(); ++i) {
			v[i] /= len;
		}
	}
};
