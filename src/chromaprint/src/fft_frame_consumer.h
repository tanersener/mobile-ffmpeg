// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_FFT_FRAME_CONSUMER_H_
#define CHROMAPRINT_FFT_FRAME_CONSUMER_H_

#include "fft_frame.h"

namespace chromaprint {

class FFTFrameConsumer
{
public:
	virtual ~FFTFrameConsumer() {}
	virtual void Consume(const FFTFrame &frame) = 0;
};

}; // namespace chromaprint

#endif
