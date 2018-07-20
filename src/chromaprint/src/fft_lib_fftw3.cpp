// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#include "fft_lib_fftw3.h"

namespace chromaprint {

FFTLib::FFTLib(size_t frame_size) : m_frame_size(frame_size) {
	m_window = (FFTW_SCALAR *) fftw_malloc(sizeof(FFTW_SCALAR) * frame_size);
	m_input = (FFTW_SCALAR *) fftw_malloc(sizeof(FFTW_SCALAR) * frame_size);
	m_output = (FFTW_SCALAR *) fftw_malloc(sizeof(FFTW_SCALAR) * frame_size);
	PrepareHammingWindow(m_window, m_window + frame_size, 1.0 / INT16_MAX);
	m_plan = fftw_plan_r2r_1d(frame_size, m_input, m_output, FFTW_R2HC, FFTW_ESTIMATE);
}

FFTLib::~FFTLib() {
	fftw_destroy_plan(m_plan);
	fftw_free(m_output);
	fftw_free(m_input);
	fftw_free(m_window);
}

void FFTLib::Load(const int16_t *b1, const int16_t *e1, const int16_t *b2, const int16_t *e2) {
	auto window = m_window;
	auto output = m_input;
	ApplyWindow(b1, e1, window, output);
	ApplyWindow(b2, e2, window, output);
}

void FFTLib::Compute(FFTFrame &frame) {
	fftw_execute(m_plan);
	auto output = frame.data();
	auto in_ptr = m_output;
	auto rev_in_ptr = m_output + m_frame_size - 1;
	output[0] = in_ptr[0] * in_ptr[0];
	output[m_frame_size / 2] = in_ptr[m_frame_size / 2] * in_ptr[m_frame_size / 2];
	in_ptr += 1;
	output += 1;
	for (size_t i = 1; i < m_frame_size / 2; i++) {
		*output++ = in_ptr[0] * in_ptr[0] + rev_in_ptr[0] * rev_in_ptr[0];
		in_ptr++;
		rev_in_ptr--;
	}
}

}; // namespace chromaprint
