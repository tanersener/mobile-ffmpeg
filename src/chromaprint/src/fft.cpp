// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#include "audio/audio_slicer.h"
#include "utils.h"
#include "fft_lib.h"
#include "fft.h"
#include "debug.h"

namespace chromaprint {

FFT::FFT(size_t frame_size, size_t overlap, FFTFrameConsumer *consumer)
	: m_frame(1 + frame_size / 2), m_slicer(frame_size, frame_size - overlap), m_lib(new FFTLib(frame_size)), m_consumer(consumer) {}

FFT::~FFT() {}

void FFT::Reset() {
	m_slicer.Reset();
}

void FFT::Consume(const int16_t *input, int length) {
	m_slicer.Process(input, input + length, [&](const int16_t *b1, const int16_t *e1, const int16_t *b2, const int16_t *e2) {
		m_lib->Load(b1, e1, b2, e2);
		m_lib->Compute(m_frame);
		m_consumer->Consume(m_frame);
	});
}

}; // namespace chromaprint
