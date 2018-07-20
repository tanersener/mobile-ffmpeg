// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_CHROMA_FILTER_H_
#define CHROMAPRINT_CHROMA_FILTER_H_

#include <vector>
#include "feature_vector_consumer.h"

namespace chromaprint {
	
class ChromaFilter : public FeatureVectorConsumer {
public:
	ChromaFilter(const double *coefficients, int length, FeatureVectorConsumer *consumer);
	~ChromaFilter();

	void Reset();
	void Consume(std::vector<double> &features);

	FeatureVectorConsumer *consumer() { return m_consumer; }
	void set_consumer(FeatureVectorConsumer *consumer) { m_consumer = consumer; }

private:
	const double *m_coefficients;
	int m_length;
	std::vector< std::vector<double> > m_buffer;
	std::vector<double> m_result;
	int m_buffer_offset;
	int m_buffer_size;
	FeatureVectorConsumer *m_consumer;
};

}; // namespace chromaprint

#endif
