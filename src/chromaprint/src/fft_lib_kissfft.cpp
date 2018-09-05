// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#include "fft_lib_kissfft.h"

namespace chromaprint {

FFTLib::FFTLib(size_t frame_size) : m_frame_size(frame_size) {
	m_window = (kiss_fft_scalar *) KISS_FFT_MALLOC(sizeof(kiss_fft_scalar) * frame_size);
	m_input = (kiss_fft_scalar *) KISS_FFT_MALLOC(sizeof(kiss_fft_scalar) * frame_size);
	m_output = (kiss_fft_cpx *) KISS_FFT_MALLOC(sizeof(kiss_fft_cpx) * frame_size);
	PrepareHammingWindow(m_window, m_window + frame_size, 1.0 / INT16_MAX);
	m_cfg = kiss_fftr_alloc(frame_size, 0, NULL, NULL);
}

FFTLib::~FFTLib() {
	kiss_fftr_free(m_cfg);
	KISS_FFT_FREE(m_output);
	KISS_FFT_FREE(m_input);
	KISS_FFT_FREE(m_window);
}

void FFTLib::Load(const int16_t *b1, const int16_t *e1, const int16_t *b2, const int16_t *e2) {
	auto window = m_window;
	auto output = m_input;
	ApplyWindow(b1, e1, window, output);
	ApplyWindow(b2, e2, window, output);
}

void FFTLib::Compute(FFTFrame &frame) {
	kiss_fftr(m_cfg, m_input, m_output);
	auto input = m_output;
	auto output = frame.data();
	for (size_t i = 0; i <= m_frame_size / 2; ++i, ++input, ++output) {
		*output = input->r * input->r + input->i * input->i;
	}
}

}; // namespace chromaprint
