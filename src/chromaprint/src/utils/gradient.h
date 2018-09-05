// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_UTILS_GRADIENT_H_
#define CHROMAPRINT_UTILS_GRADIENT_H_

#include <algorithm>

namespace chromaprint {

template <typename InputIt, typename OutputIt>
void Gradient(InputIt begin, InputIt end, OutputIt output) {
	typedef typename std::iterator_traits<InputIt>::value_type ValueType;
	InputIt ptr = begin;
	if (ptr != end) {
		ValueType f0 = *ptr++;
		if (ptr == end) {
			*output = 0;
			return;
		}
		ValueType f1 = *ptr++;
		*output++ = f1 - f0;
		if (ptr == end) {
			*output = f1 - f0;
			return;
		}
		ValueType f2 = *ptr++;
		while (true) {
			*output++ = (f2 - f0) / 2;
			if (ptr == end) {
				*output = f2 - f1;
				return;
			}
			f0 = f1;
			f1 = f2;
			f2 = *ptr++;
		};
	}
}

};

#endif
