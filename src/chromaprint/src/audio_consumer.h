// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_AUDIO_CONSUMER_H_
#define CHROMAPRINT_AUDIO_CONSUMER_H_

namespace chromaprint {

class AudioConsumer {
public:
	virtual ~AudioConsumer() {}
	virtual void Consume(const int16_t *input, int length) = 0;
};

}; // namespace chromaprint

#endif
