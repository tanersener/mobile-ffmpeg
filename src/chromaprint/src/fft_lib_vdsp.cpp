// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#include <cassert>
#include "fft_lib_vdsp.h"

namespace chromaprint {

FFTLib::FFTLib(size_t frame_size) : m_frame_size(frame_size) {
	double log2n = log2(frame_size);
	assert(log2n == int(log2n));
	m_log2n = int(log2n);
	m_window = new float[frame_size];
	m_input = new float[frame_size];
	m_a.realp = new float[frame_size / 2];
	m_a.imagp = new float[frame_size / 2];
	PrepareHammingWindow(m_window, m_window + frame_size, 0.5 / INT16_MAX);
	m_setup = vDSP_create_fftsetup(m_log2n, 0);
}

FFTLib::~FFTLib() {
	vDSP_destroy_fftsetup(m_setup);
	delete[] m_a.realp;
	delete[] m_a.imagp;
	delete[] m_input;
	delete[] m_window;
}

void FFTLib::Load(const int16_t *b1, const int16_t *e1, const int16_t *b2, const int16_t *e2) {
	auto window = m_window;
	auto output = m_input;
	ApplyWindow(b1, e1, window, output);
	ApplyWindow(b2, e2, window, output);
}

void FFTLib::Compute(FFTFrame &frame) {
	vDSP_ctoz((DSPComplex *) m_input, 2, &m_a, 1, m_frame_size / 2); 
	vDSP_fft_zrip(m_setup, &m_a, 1, m_log2n, FFT_FORWARD);
	auto output = frame.data();
	output[0] = m_a.realp[0] * m_a.realp[0];
	output[m_frame_size / 2] = m_a.imagp[0] * m_a.imagp[0];
	output += 1;
	for (size_t i = 1; i < m_frame_size / 2; ++i, ++output) {
		*output = m_a.realp[i] * m_a.realp[i] + m_a.imagp[i] * m_a.imagp[i];
	}
}

}; // namespace chromaprint
