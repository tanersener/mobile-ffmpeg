// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_CLASSIFIER_H_
#define CHROMAPRINT_CLASSIFIER_H_

#include <ostream>
#include "quantizer.h"
#include "filter.h"

namespace chromaprint {

class Classifier
{
public:
	Classifier(const Filter &filter = Filter(), const Quantizer &quantizer = Quantizer())
		: m_filter(filter), m_quantizer(quantizer) { }

	template <typename IntegralImage>
	int Classify(const IntegralImage &image, size_t offset) const {
		double value = m_filter.Apply(image, offset);
		return m_quantizer.Quantize(value);
	}

	const Filter &filter() const { return m_filter; }
	const Quantizer &quantizer() const { return m_quantizer; }

private:
	Filter m_filter;
	Quantizer m_quantizer;
};

inline std::ostream &operator<<(std::ostream &stream, const Classifier &q) {
	stream << "Classifier(" << q.filter() << ", " << q.quantizer() << ")";
	return stream;
}

}; // namespace chromaprint

#endif
