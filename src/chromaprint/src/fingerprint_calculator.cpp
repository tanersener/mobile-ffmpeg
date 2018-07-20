// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#include "fingerprint_calculator.h"
#include "classifier.h"
#include "debug.h"
#include "utils.h"

namespace chromaprint {

FingerprintCalculator::FingerprintCalculator(const Classifier *classifiers, size_t num_classifiers)
	: m_classifiers(classifiers), m_num_classifiers(num_classifiers), m_image(256)
{
	m_max_filter_width = 0;
	for (size_t i = 0; i < num_classifiers; i++) {
		m_max_filter_width = std::max(m_max_filter_width, (size_t) classifiers[i].filter().width());
	}
	assert(m_max_filter_width > 0);
	assert(m_max_filter_width < 256);
}

uint32_t FingerprintCalculator::CalculateSubfingerprint(size_t offset)
{
	uint32_t bits = 0;
	for (size_t i = 0; i < m_num_classifiers; i++) {
		bits = (bits << 2) | GrayCode(m_classifiers[i].Classify(m_image, offset));
	}
	return bits;
}

void FingerprintCalculator::Reset() {
	m_image.Reset();
	m_fingerprint.clear();
}

void FingerprintCalculator::Consume(std::vector<double> &features) {
	m_image.AddRow(features);
	if (m_image.num_rows() >= m_max_filter_width) {
		m_fingerprint.push_back(CalculateSubfingerprint(m_image.num_rows() - m_max_filter_width));
	}
}

const std::vector<uint32_t> &FingerprintCalculator::GetFingerprint() const {
	return m_fingerprint;
}

void FingerprintCalculator::ClearFingerprint() {
	m_fingerprint.clear();
}

}; // namespace chromaprint
