// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_UTILS_GAUSSIAN_FILTER_H_
#define CHROMAPRINT_UTILS_GAUSSIAN_FILTER_H_

#include <cmath>
#include "debug.h"

namespace chromaprint {

struct ReflectIterator {
	ReflectIterator(size_t size) : size(size) {}

	void MoveForward() {
		if (forward) {
			if (pos + 1 == size) {
				forward = false;
			} else {
				pos++;
			}
		} else {
			if (pos == 0) {
				forward = true;
			} else {
				pos--;
			}
		}
	}

	void MoveBack() {
		if (forward) {
			if (pos == 0) {
				forward = false;
			} else {
				pos--;
			}
		} else {
			if (pos + 1 == size) {
				forward = true;
			} else {
				pos++;
			}
		}
	}

	size_t SafeForwardDistance() {
		if (forward) {
			return size - pos - 1;
		}
		return 0;
	}

	size_t size;
	size_t pos { 0 };
	bool forward { true };
};

template <typename T>
void BoxFilter(T &input, T &output, size_t w) {
	const size_t size = input.size();
	output.resize(size);
	if (w == 0 || size == 0) {
		return;
	}

	const size_t wl = w / 2;
	const size_t wr = w - wl;

	auto out = output.begin();

	ReflectIterator it1(size), it2(size);
	for (size_t i = 0; i < wl; i++) {
		it1.MoveBack();
		it2.MoveBack();
	}

	typename T::value_type sum = 0;
	for (size_t i = 0; i < w; i++) {
		sum += input[it2.pos];
		it2.MoveForward();
	}

	if (size > w) {
		for (size_t i = 0; i < wl; i++) {
			*out++ = sum / w;
			sum += input[it2.pos] - input[it1.pos];
			it1.MoveForward();
			it2.MoveForward();
		}
		for (size_t i = 0; i < size - w - 1; i++) {
			*out++ = sum / w;
			sum += input[it2.pos++] - input[it1.pos++];
		}
		for (size_t i = 0; i < wr + 1; i++) {
			*out++ = sum / w;
			sum += input[it2.pos] - input[it1.pos];
			it1.MoveForward();
			it2.MoveForward();
		}
	} else {
		for (size_t i = 0; i < size; i++) {
			*out++ = sum / w;
			sum += input[it2.pos] - input[it1.pos];
			it1.MoveForward();
			it2.MoveForward();
		}
	}
};

template <typename T>
void GaussianFilter(T &input, T& output, double sigma, int n) {
	const int w = floor(sqrt(12 * sigma * sigma / n + 1));
	const int wl = w - (w % 2 == 0 ? 1 : 0);
	const int wu = wl + 2;

	const int m = round((12 * sigma * sigma - n * wl * wl - 4 * n * wl - 3 * n) / (-4 * wl - 4));

	T* data1 = &input;
	T* data2 = &output;

	int i = 0;
	for (; i < m; i++) {
		BoxFilter(*data1, *data2, wl);
		std::swap(data1, data2);
	}
	for (; i < n; i++) {
		BoxFilter(*data1, *data2, wu);
		std::swap(data1, data2);
	}

	if (data1 != &output) {
		output.assign(input.begin(), input.end());
	}
}

};

#endif
