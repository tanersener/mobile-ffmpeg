// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_CHROMA_NORMALIZER_H_
#define CHROMAPRINT_CHROMA_NORMALIZER_H_

#include <vector>
#include <algorithm>
#include "feature_vector_consumer.h"
#include "utils.h"

namespace chromaprint {

class ChromaNormalizer : public FeatureVectorConsumer {
public:
	ChromaNormalizer(FeatureVectorConsumer *consumer) : m_consumer(consumer) {}
	~ChromaNormalizer() {}
	void Reset() {}

	void Consume(std::vector<double> &features)
	{
		NormalizeVector(features.begin(), features.end(),
						chromaprint::EuclideanNorm<std::vector<double>::iterator>,
						0.01);
		m_consumer->Consume(features);
	}

private:
	CHROMAPRINT_DISABLE_COPY(ChromaNormalizer);

	FeatureVectorConsumer *m_consumer;
};

}; // namespace chromaprint

#endif
