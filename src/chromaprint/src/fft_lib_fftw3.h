// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_FFT_LIB_FFTW3_H_
#define CHROMAPRINT_FFT_LIB_FFTW3_H_

#include <fftw3.h>

#include "fft_frame.h"
#include "utils.h"

#ifdef USE_FFTW3F
#define FFTW_SCALAR float
#define fftw_plan fftwf_plan
#define fftw_plan_r2r_1d fftwf_plan_r2r_1d
#define fftw_execute fftwf_execute
#define fftw_destroy_plan fftwf_destroy_plan
#define fftw_malloc fftwf_malloc
#define fftw_free fftwf_free
#else
#define FFTW_SCALAR double
#endif

namespace chromaprint {

class FFTLib {
public:
	FFTLib(size_t frame_size);
	~FFTLib();

	void Load(const int16_t *begin1, const int16_t *end1, const int16_t *begin2, const int16_t *end2);
	void Compute(FFTFrame &frame);

private:
	CHROMAPRINT_DISABLE_COPY(FFTLib);

	size_t m_frame_size;
	FFTW_SCALAR *m_window;
	FFTW_SCALAR *m_input;
	FFTW_SCALAR *m_output;
	fftw_plan m_plan;
};

}; // namespace chromaprint

#endif // CHROMAPRINT_FFT_LIB_FFTW3_H_
