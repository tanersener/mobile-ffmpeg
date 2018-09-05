// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#include "fft_lib_avfft.h"

namespace chromaprint {

FFTLib::FFTLib(size_t frame_size) : m_frame_size(frame_size) {
	m_window = (FFTSample *) av_malloc(sizeof(FFTSample) * frame_size);
	m_input = (FFTSample *) av_malloc(sizeof(FFTSample) * frame_size);
	PrepareHammingWindow(m_window, m_window + frame_size, 1.0 / INT16_MAX);
	int bits = -1;
	while (frame_size) {
		bits++;
		frame_size >>= 1;
	}
	m_rdft_ctx = av_rdft_init(bits, DFT_R2C);
}

FFTLib::~FFTLib() {
	av_rdft_end(m_rdft_ctx);
	av_free(m_input);
	av_free(m_window);
}

void FFTLib::Load(const int16_t *b1, const int16_t *e1, const int16_t *b2, const int16_t *e2) {
	auto window = m_window;
	auto output = m_input;
	ApplyWindow(b1, e1, window, output);
	ApplyWindow(b2, e2, window, output);
}

void FFTLib::Compute(FFTFrame &frame) {
	av_rdft_calc(m_rdft_ctx, m_input);
	auto input = m_input;
	auto output = frame.begin();
	output[0] = input[0] * input[0];
	output[m_frame_size / 2] = input[1] * input[1];
	output += 1;
	input += 2;
	for (size_t i = 1; i < m_frame_size / 2; i++) {
		*output++ = input[0] * input[0] + input[1] * input[1];
		input += 2;
	}
}

}; // namespace chromaprint
