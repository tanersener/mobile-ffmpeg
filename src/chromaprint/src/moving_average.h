// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_MOVING_AVERAGE_H_
#define CHROMAPRINT_MOVING_AVERAGE_H_

#include <vector>

namespace chromaprint {

template<class T>
class MovingAverage
{
public:
	MovingAverage(int size)
		: m_buffer(size), m_size(size), m_offset(0), m_sum(0), m_count(0) {}

	void AddValue(const T &x)
	{
		m_sum += x;
		m_sum -= m_buffer[m_offset];
		if (m_count < m_size) {
			m_count++;
		}
		m_buffer[m_offset] = x;
		m_offset = (m_offset + 1) % m_size;
	}

	T GetAverage() const
	{
		if (!m_count) {
			return 0;
		}
		return m_sum / m_count;
	}

private:
	std::vector<T> m_buffer;
	int m_size;
	int m_offset;
	int m_sum;
	int m_count;
};

}; // namespace chromaprint

#endif
