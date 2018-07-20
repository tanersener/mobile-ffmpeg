// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#include <limits>
#include <cassert>
#include <cmath>
#include "chroma_resampler.h"
#include "utils.h"

namespace chromaprint {

ChromaResampler::ChromaResampler(int factor, FeatureVectorConsumer *consumer)
	: m_result(12, 0.0),
	  m_iteration(0),
	  m_factor(factor),
	  m_consumer(consumer)
{
}

ChromaResampler::~ChromaResampler()
{
}

void ChromaResampler::Reset()
{
	m_iteration = 0;
	fill(m_result.begin(), m_result.end(), 0.0);
}

void ChromaResampler::Consume(std::vector<double> &features)
{
	for (int i = 0; i < 12; i++) {
		m_result[i] += features[i];
	}
	m_iteration += 1;
	if (m_iteration == m_factor) {
		for (int i = 0; i < 12; i++) {
			m_result[i] /= m_factor;
		}
		m_consumer->Consume(m_result);
		Reset();
	}
}

}; // namespace chromaprint
