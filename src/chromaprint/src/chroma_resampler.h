// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_CHROMA_RESAMPLER_H_
#define CHROMAPRINT_CHROMA_RESAMPLER_H_

#include <vector>
#include "image.h"
#include "feature_vector_consumer.h"

namespace chromaprint {
	
class ChromaResampler : public FeatureVectorConsumer {
public:
	ChromaResampler(int factor, FeatureVectorConsumer *consumer);
	~ChromaResampler();

	void Reset();
	void Consume(std::vector<double> &features);

	FeatureVectorConsumer *consumer() { return m_consumer; }
	void set_consumer(FeatureVectorConsumer *consumer) { m_consumer = consumer; }

private:
	std::vector<double> m_result;
	int m_iteration;
	int m_factor;
	FeatureVectorConsumer *m_consumer;
};

}; // namespace chromaprint

#endif
