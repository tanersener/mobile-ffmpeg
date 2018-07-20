// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_FILTER_UTILS_H_
#define CHROMAPRINT_FILTER_UTILS_H_

#include <cmath>
#include <cassert>
#include "utils.h"

namespace chromaprint {

inline double Subtract(double a, double b) {
	return a - b;
}

inline double SubtractLog(double a, double b) {
	double r = log((1.0 + a) / (1.0 + b));
	assert(!IsNaN(r));
	return r;
}

// oooooooooooooooo
// oooooooooooooooo
// oooooooooooooooo
// oooooooooooooooo
template <typename IntegralImage, typename Comparator>
double Filter0(const IntegralImage &image, size_t x, size_t y, size_t w, size_t h, Comparator cmp) {
	assert(w >= 1);
	assert(h >= 1);

	double a = image.Area(x, y, x + w, y + h);
	double b = 0;

	return cmp(a, b);
}

// ................
// ................
// oooooooooooooooo
// oooooooooooooooo
template <typename IntegralImage, typename Comparator>
double Filter1(const IntegralImage &image, size_t x, size_t y, size_t w, size_t h, Comparator cmp) {
	assert(w >= 1);
	assert(h >= 1);

	const auto h_2 = h / 2;

	double a = image.Area(x, y + h_2, x + w, y + h  );
	double b = image.Area(x, y,       x + w, y + h_2);

	return cmp(a, b);
}

// .......ooooooooo
// .......ooooooooo
// .......ooooooooo
// .......ooooooooo
template <typename IntegralImage, typename Comparator>
double Filter2(const IntegralImage &image, size_t x, size_t y, size_t w, size_t h, Comparator cmp) {
	assert(w >= 1);
	assert(h >= 1);

	const auto w_2 = w / 2;

	double a = image.Area(x + w_2, y, x + w  , y + h);
	double b = image.Area(x,       y, x + w_2, y + h);

	return cmp(a, b);
}

// .......ooooooooo
// .......ooooooooo
// ooooooo.........
// ooooooo.........
template <typename IntegralImage, typename Comparator>
double Filter3(const IntegralImage &image, size_t x, size_t y, size_t w, size_t h, Comparator cmp) {
	assert(x >= 0);
	assert(y >= 0);
	assert(w >= 1);
	assert(h >= 1);

	const auto w_2 = w / 2;
	const auto h_2 = h / 2;

	double a = image.Area(x,       y + h_2, x + w_2, y + h  ) +
			   image.Area(x + w_2, y,       x + w  , y + h_2);
	double b = image.Area(x,       y,       x + w_2, y + h_2) +
			   image.Area(x + w_2, y + h_2, x + w  , y + h  );

	return cmp(a, b);
}

// ................
// oooooooooooooooo
// ................
template <typename IntegralImage, typename Comparator>
double Filter4(const IntegralImage &image, size_t x, size_t y, size_t w, size_t h, Comparator cmp) {
	assert(w >= 1);
	assert(h >= 1);

	const auto h_3 = h / 3;

	double a = image.Area(x, y + h_3,     x + w, y + 2 * h_3);
	double b = image.Area(x, y,           x + w, y + h_3) +
			   image.Area(x, y + 2 * h_3, x + w, y + h);

	return cmp(a, b);
}

// .....oooooo.....
// .....oooooo.....
// .....oooooo.....
// .....oooooo.....
template <typename IntegralImage, typename Comparator>
double Filter5(const IntegralImage &image, size_t x, size_t y, size_t w, size_t h, Comparator cmp) {
	assert(w >= 1);
	assert(h >= 1);

	const auto w_3 = w / 3;

	double a = image.Area(x + w_3,     y, x + 2 * w_3, y + h);
	double b = image.Area(x,           y, x + w_3,     y + h) +
			   image.Area(x + 2 * w_3, y, x + w,       y + h);

	return cmp(a, b);
}

}; // namespace chromaprint

#endif
