// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#include <limits>
#include <math.h>
#include "fft_frame.h"
#include "utils.h"
#include "spectrum.h"

namespace chromaprint {

Spectrum::Spectrum(int num_bands, int min_freq, int max_freq, int frame_size, int sample_rate, FeatureVectorConsumer *consumer)
	: m_bands(num_bands + 1),
	  m_features(num_bands),
	  m_consumer(consumer)
{
	PrepareBands(num_bands, min_freq, max_freq, frame_size, sample_rate);
}

Spectrum::~Spectrum()
{
}

void Spectrum::PrepareBands(int num_bands, int min_freq, int max_freq, int frame_size, int sample_rate)
{
    double min_bark = FreqToBark(min_freq);
    double max_bark = FreqToBark(max_freq);
    double band_size = (max_bark - min_bark) / num_bands;

    int min_index = FreqToIndex(min_freq, frame_size, sample_rate);
    //int max_index = FreqToIndex(max_freq, frame_size, sample_rate);

    m_bands[0] = min_index;
    double prev_bark = min_bark;

    for (int i = min_index, b = 0; i < frame_size / 2; i++) {
        double freq = IndexToFreq(i, frame_size, sample_rate);
        double bark = FreqToBark(freq);
        if (bark - prev_bark > band_size) {
            b += 1;
            prev_bark = bark;
            m_bands[b] = i;
            if (b >= num_bands) {
                break;
            }
        }
    }
}

void Spectrum::Reset()
{
}

void Spectrum::Consume(const FFTFrame &frame)
{
	for (int i = 0; i < NumBands(); i++) {
		int first = FirstIndex(i);
		int last = LastIndex(i);
		double numerator = 0.0;
		double denominator = 0.0;
		for (int j = first; j < last; j++) {
			double s = frame[j];
			numerator += j * s;
			denominator += s;
		}
		m_features[i] = denominator / (last - first);
	}
	m_consumer->Consume(m_features);
}

}; // namespace chromaprint
