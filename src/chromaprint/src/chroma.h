// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_CHROMA_H_
#define CHROMAPRINT_CHROMA_H_

#include <math.h>
#include <vector>
#include "utils.h"
#include "fft_frame_consumer.h"
#include "feature_vector_consumer.h"

namespace chromaprint {

class Chroma : public FFTFrameConsumer {
public:
	Chroma(int min_freq, int max_freq, int frame_size, int sample_rate, FeatureVectorConsumer *consumer);
	~Chroma();

	bool interpolate() const {
		return m_interpolate;
	}

	void set_interpolate(bool interpolate) {
		m_interpolate = interpolate;
	}

	void Reset();
	void Consume(const FFTFrame &frame);

private:
	CHROMAPRINT_DISABLE_COPY(Chroma);

	void PrepareNotes(int min_freq, int max_freq, int frame_size, int sample_rate);

	bool m_interpolate;
	std::vector<char> m_notes;
	std::vector<double> m_notes_frac;
	int m_min_index;
	int m_max_index;
	std::vector<double> m_features;
	FeatureVectorConsumer *m_consumer;
};

}; // namespace chromaprint

#endif
