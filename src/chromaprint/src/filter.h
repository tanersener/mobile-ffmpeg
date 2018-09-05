// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_FILTER_H_
#define CHROMAPRINT_FILTER_H_

#include <ostream>
#include "filter_utils.h"

namespace chromaprint {

class Filter {
public:
	Filter(int type = 0, int y = 0, int height = 0, int width = 0)
		: m_type(type), m_y(y), m_height(height), m_width(width) { }

	template <typename IntegralImage>
	double Apply(const IntegralImage &image, size_t x) const {
		switch (m_type) {
			case 0:
				return Filter0(image, x, m_y, m_width, m_height, SubtractLog);
			case 1:
				return Filter1(image, x, m_y, m_width, m_height, SubtractLog);
			case 2:
				return Filter2(image, x, m_y, m_width, m_height, SubtractLog);
			case 3:
				return Filter3(image, x, m_y, m_width, m_height, SubtractLog);
			case 4:
				return Filter4(image, x, m_y, m_width, m_height, SubtractLog);
			case 5:
				return Filter5(image, x, m_y, m_width, m_height, SubtractLog);
		}
		return 0.0;
	}

	int type() const { return m_type; }
	void set_type(int type) { m_type = type; }

	int y() const { return m_y; }
	void set_y(int y) { m_y = y; }

	int height() const { return m_height; }
	void set_height(int height) { m_height = height; }

	int width() const { return m_width; }
	void set_width(int width) { m_width = width; }

private:
	int m_type;
	int m_y;
	int m_height;
	int m_width;
};

inline std::ostream &operator<<(std::ostream &stream, const Filter &f) {
	stream << "Filter(" << f.type() << ", " << f.y() << ", "
		<< f.height() << ", " << f.width() << ")";
	return stream;
}
	
}; // namespace chromaprint

#endif
