// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_SILENCE_REMOVER_H_
#define CHROMAPRINT_SILENCE_REMOVER_H_

#include "utils.h"
#include "audio_consumer.h"
#include "moving_average.h"

namespace chromaprint {

class SilenceRemover : public AudioConsumer
{
public:
	SilenceRemover(AudioConsumer *consumer, int threshold = 0);

	AudioConsumer *consumer() const
	{
		return m_consumer;
	}

	void set_consumer(AudioConsumer *consumer)
	{
		m_consumer = consumer;
	}

	bool Reset(int sample_rate, int num_channels);
	void Consume(const int16_t *input, int length) override;
	void Flush();

	int threshold()
	{
		return m_threshold;
	}

	void set_threshold(int value)
	{
		m_threshold = value;
	}

private:
	CHROMAPRINT_DISABLE_COPY(SilenceRemover);

	bool m_start;
	int m_threshold;
	MovingAverage<int16_t> m_average;
	AudioConsumer *m_consumer;
};

}; // namespace chromaprint

#endif
