// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_SPECTRUM_H_
#define CHROMAPRINT_SPECTRUM_H_

#include <math.h>
#include <vector>
#include "utils.h"
#include "fft_frame_consumer.h"
#include "feature_vector_consumer.h"

namespace chromaprint {

class Spectrum : public FFTFrameConsumer
{
public:
	Spectrum(int num_bands, int min_freq, int max_freq, int frame_size, int sample_rate, FeatureVectorConsumer *consumer);
	~Spectrum();

	void Reset();
	void Consume(const FFTFrame &frame);

protected:
	int NumBands() const { return m_bands.size() - 1; }
	int FirstIndex(int band) const { return m_bands[band]; }
	int LastIndex(int band) const { return m_bands[band + 1]; }

private:
	CHROMAPRINT_DISABLE_COPY(Spectrum);
	
	void PrepareBands(int num_bands, int min_freq, int max_freq, int frame_size, int sample_rate);

	std::vector<int> m_bands;
	std::vector<double> m_features;
	FeatureVectorConsumer *m_consumer;
};

}; // namespace chromaprint

#endif
