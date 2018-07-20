// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_FFT_LIB_VDSP_H_
#define CHROMAPRINT_FFT_LIB_VDSP_H_

#include <Accelerate/Accelerate.h>

#include "fft_frame.h"
#include "utils.h"

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
	float *m_window;
	float *m_input;
	int m_log2n;
	FFTSetup m_setup;
	DSPSplitComplex m_a;
};

}; // namespace chromaprint

#endif // CHROMAPRINT_FFT_LIB_VDSP_H_
