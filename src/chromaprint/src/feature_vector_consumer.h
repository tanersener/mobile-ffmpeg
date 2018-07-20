// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_FEATURE_VECTOR_CONSUMER_H_
#define CHROMAPRINT_FEATURE_VECTOR_CONSUMER_H_

#include <vector>

namespace chromaprint {

class FeatureVectorConsumer {
public:
	virtual ~FeatureVectorConsumer() {}
	virtual void Consume(std::vector<double> &features) = 0;
};

}; // namespace chromaprint

#endif
