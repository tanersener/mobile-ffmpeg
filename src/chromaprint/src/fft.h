// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_FFT_H_
#define CHROMAPRINT_FFT_H_

#include <cmath>
#include <memory>
#include "utils.h"
#include "fft_frame.h"
#include "fft_frame_consumer.h"
#include "audio_consumer.h"
#include "audio/audio_slicer.h"

namespace chromaprint {

class FFTLib;

class FFT : public AudioConsumer
{
public:
	FFT(size_t frame_size, size_t overlap, FFTFrameConsumer *consumer);
	~FFT();

	size_t frame_size() const {
		return m_slicer.size();
	}

	size_t increment() const {
		return m_slicer.increment();
	}

	size_t overlap() const {
		return m_slicer.size() - m_slicer.increment();
	}

	void Reset();
	void Consume(const int16_t *input, int length) override;

private:
	CHROMAPRINT_DISABLE_COPY(FFT);

	FFTFrame m_frame;
	AudioSlicer<int16_t> m_slicer;
	std::unique_ptr<FFTLib> m_lib;
	FFTFrameConsumer *m_consumer;
};

}; // namespace chromaprint

#endif
