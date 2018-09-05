// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#include <cassert>
#include <algorithm>
#include "debug.h"
#include "silence_remover.h"

namespace chromaprint {

const short kSilenceWindow = 55; // 5 ms as 11025 Hz

SilenceRemover::SilenceRemover(AudioConsumer *consumer, int threshold)
    : m_start(true),
	  m_threshold(threshold),
	  m_average(kSilenceWindow),
	  m_consumer(consumer)
{
}

bool SilenceRemover::Reset(int sample_rate, int num_channels)
{
	if (num_channels != 1) {
		DEBUG("chromaprint::SilenceRemover::Reset() -- Expecting mono audio signal.");
		return false;
	}
	m_start = true;
	return true;
}

void SilenceRemover::Consume(const int16_t *input, int length)
{
	if (m_start) {
		while (length) {
			m_average.AddValue(std::abs(*input));
			if (m_average.GetAverage() > m_threshold) {
				m_start = false;
				break;
			}
			input++;
			length--;
		}
	}
	if (length) {
		m_consumer->Consume(input, length);
	}
}

void SilenceRemover::Flush()
{
}

}; // namespace chromaprint
