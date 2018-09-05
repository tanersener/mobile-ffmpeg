// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#include <limits>
#include <assert.h>
#include <math.h>
#include "chroma_filter.h"
#include "utils.h"

namespace chromaprint {

ChromaFilter::ChromaFilter(const double *coefficients, int length, FeatureVectorConsumer *consumer)
	: m_coefficients(coefficients),
	  m_length(length),
	  m_buffer(8),
	  m_result(12),
	  m_buffer_offset(0),
	  m_buffer_size(1),
	  m_consumer(consumer)
{
}

ChromaFilter::~ChromaFilter()
{
}

void ChromaFilter::Reset()
{
	m_buffer_size = 1;
	m_buffer_offset = 0;
}

void ChromaFilter::Consume(std::vector<double> &features)
{
	m_buffer[m_buffer_offset] = features;
	m_buffer_offset = (m_buffer_offset + 1) % 8;
	if (m_buffer_size >= m_length) {
		int offset = (m_buffer_offset + 8 - m_length) % 8;
		fill(m_result.begin(), m_result.end(), 0.0);
		for (int i = 0; i < 12; i++) {
			for (int j = 0; j < m_length; j++) {
				m_result[i] += m_buffer[(offset + j) % 8][i] * m_coefficients[j];
			}
		}
		m_consumer->Consume(m_result);
	}
	else {
		m_buffer_size++;
	}
}

}; // namespace chromaprint
