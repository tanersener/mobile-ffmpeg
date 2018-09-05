// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_FINGERPRINT_CALCULATOR_H_
#define CHROMAPRINT_FINGERPRINT_CALCULATOR_H_

#include <cstdint>
#include <vector>
#include "feature_vector_consumer.h"
#include "utils/rolling_integral_image.h"

namespace chromaprint {

class Classifier;
class Image;
class IntegralImage;

class FingerprintCalculator : public FeatureVectorConsumer {
public:
	FingerprintCalculator(const Classifier *classifiers, size_t num_classifiers);

	virtual void Consume(std::vector<double> &features) override;

	//! Get the fingerprint generate from data up to this point.
	const std::vector<uint32_t> &GetFingerprint() const;

	//! Clear the generated fingerprint, but allow more features to be processed.
	void ClearFingerprint();

	//! Reset all internal state.
	void Reset();

private:
	uint32_t CalculateSubfingerprint(size_t offset);

	const Classifier *m_classifiers;
	size_t m_num_classifiers;
	size_t m_max_filter_width;
	RollingIntegralImage m_image;
	std::vector<uint32_t> m_fingerprint;
};

}; // namespace chromaprint

#endif

