// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_ROLLING_INTEGRAL_IMAGE_H_
#define CHROMAPRINT_ROLLING_INTEGRAL_IMAGE_H_

#include <cstddef>
#include <cassert>
#include <algorithm>
#include <numeric>
#include "debug.h"

namespace chromaprint {

class RollingIntegralImage {
public:

	explicit RollingIntegralImage(size_t max_rows) : m_max_rows(max_rows + 1) {}

	template <typename InputIt>
	RollingIntegralImage(size_t num_columns, InputIt begin, InputIt end) {
		m_max_rows = std::distance(begin, end) / num_columns;
		while (begin != end) {
			AddRow(begin, begin + num_columns);
			std::advance(begin, num_columns);
		}
	}

	size_t num_columns() const { return m_num_columns; }
	size_t num_rows() const { return m_num_rows; }

	void Reset() {
		m_data.clear();
		m_num_rows = 0;
		m_num_columns = 0;
	}

	double Area(size_t r1, size_t c1, size_t r2, size_t c2) const {
		assert(r1 <= m_num_rows);
		assert(r2 <= m_num_rows);

		if (m_num_rows > m_max_rows) {
			assert(r1 > m_num_rows - m_max_rows);
			assert(r2 > m_num_rows - m_max_rows);
		}

		assert(c1 <= m_num_columns);
		assert(c2 <= m_num_columns);

		if (r1 == r2 || c1 == c2) {
			return 0.0;
		}

		assert(r2 > r1);
		assert(c2 > c1);

		if (r1 == 0) {
			auto row = GetRow(r2 - 1);
			if (c1 == 0) {
				return row[c2 - 1];
			} else {
				return row[c2 - 1] - row[c1 - 1];
			}
		} else {
			auto row1 = GetRow(r1 - 1);
			auto row2 = GetRow(r2 - 1);
			if (c1 == 0) {
				return row2[c2 - 1] - row1[c2 - 1];
			} else {
				return row2[c2 - 1] - row1[c2 - 1] - row2[c1 - 1] + row1[c1 - 1];
			}
		}
	}

	template <typename InputIt>
	void AddRow(InputIt begin, InputIt end) {
		const size_t size = std::distance(begin, end);
		if (m_num_columns == 0) {
			m_num_columns = size;
			m_data.resize(m_max_rows * m_num_columns, 0.0);
		}

		assert(m_num_columns == size);

		auto current_row_begin = GetRow(m_num_rows);
		std::partial_sum(begin, end, current_row_begin);

		if (m_num_rows > 0) {
			auto last_row_begin = GetRow(m_num_rows - 1);
			auto last_row_end = last_row_begin + m_num_columns;
			std::transform(last_row_begin, last_row_end, current_row_begin, current_row_begin,
				[](double a, double b) { return a + b; });
		}

		m_num_rows++;
	}

	void AddRow(const std::vector<double> &row) {
		AddRow(row.begin(), row.end());
	}

private:

	std::vector<double>::iterator GetRow(size_t i) {
		i = i % m_max_rows;
		return m_data.begin() + i * m_num_columns;
	}

	std::vector<double>::const_iterator GetRow(size_t i) const {
		i = i % m_max_rows;
		return m_data.begin() + i * m_num_columns;
	}

	size_t m_max_rows;
	size_t m_num_columns = 0;
	size_t m_num_rows = 0;
	std::vector<double> m_data;
};

}; // namespace chromaprint

#endif
