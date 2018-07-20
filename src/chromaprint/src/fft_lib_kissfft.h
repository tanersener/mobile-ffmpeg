// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_FFT_LIB_KISSFFT_H_
#define CHROMAPRINT_FFT_LIB_KISSFFT_H_

#include <tools/kiss_fftr.h>

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
	kiss_fft_scalar *m_window;
	kiss_fft_scalar *m_input;
	kiss_fft_cpx *m_output;
	kiss_fftr_cfg m_cfg;
};

}; // namespace chromaprint

#endif // CHROMAPRINT_FFT_LIB_KISSFFT_H_
