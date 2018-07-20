// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#include <assert.h>
#include <algorithm>
#include <stdio.h>
extern "C" {
#include "avresample/avcodec.h"
}
#include "debug.h"
#include "audio_processor.h"

namespace chromaprint {

static const int kMinSampleRate = 1000;
static const int kMaxBufferSize = 1024 * 32;

// Resampler configuration
static const int kResampleFilterLength = 16;
static const int kResamplePhaseShift = 8;
static const int kResampleLinear = 0;
static const double kResampleCutoff = 0.8;

AudioProcessor::AudioProcessor(int sample_rate, AudioConsumer *consumer)
	: m_buffer(kMaxBufferSize),
	  m_buffer_offset(0),
	  m_resample_buffer(kMaxBufferSize),
	  m_target_sample_rate(sample_rate),
	  m_consumer(consumer),
	  m_resample_ctx(0)
{
}

AudioProcessor::~AudioProcessor()
{
	if (m_resample_ctx) {
		av_resample_close(m_resample_ctx);
	}
}

void AudioProcessor::LoadMono(const int16_t *input, int length)
{
	int16_t *output = m_buffer.data() + m_buffer_offset;
	while (length--) {
		*output++ = input[0];
		input++;
	}
}

void AudioProcessor::LoadStereo(const int16_t *input, int length)
{
	int16_t *output = m_buffer.data() + m_buffer_offset;
	while (length--) {
		*output++ = (input[0] + input[1]) / 2;
		input += 2;
	}
}

void AudioProcessor::LoadMultiChannel(const int16_t *input, int length)
{
	int16_t *output = m_buffer.data() + m_buffer_offset;
	while (length--) {
		int32_t sum = 0;
		for (int i = 0; i < m_num_channels; i++) {
			sum += *input++;
		}
		*output++ = (int16_t)(sum / m_num_channels);
	}
}

int AudioProcessor::Load(const int16_t *input, int length)
{
	assert(length >= 0);
	assert(m_buffer_offset <= m_buffer.size());
	length = std::min(length, static_cast<int>(m_buffer.size() - m_buffer_offset));
	switch (m_num_channels) {
	case 1:
		LoadMono(input, length);
		break;
	case 2:
		LoadStereo(input, length);
		break;
	default:
		LoadMultiChannel(input, length);
		break;
	}
	m_buffer_offset += length;
	return length;
}

void AudioProcessor::Resample()
{
	if (!m_resample_ctx) {
		m_consumer->Consume(m_buffer.data(), m_buffer_offset);
		m_buffer_offset = 0;
		return;
	}
	int consumed = 0;
	int length = av_resample(m_resample_ctx, m_resample_buffer.data(), m_buffer.data(), &consumed, m_buffer_offset, kMaxBufferSize, 1);
	if (length > kMaxBufferSize) {
		DEBUG("chromaprint::AudioProcessor::Resample() -- Resampling overwrote output buffer.");
		length = kMaxBufferSize;
	}
	m_consumer->Consume(m_resample_buffer.data(), length);
	int remaining = m_buffer_offset - consumed;
	if (remaining > 0) {
		std::copy(m_buffer.begin() + consumed, m_buffer.begin() + m_buffer_offset, m_buffer.begin());
	}
	else if (remaining < 0) {
		DEBUG("chromaprint::AudioProcessor::Resample() -- Resampling overread input buffer.");
		remaining = 0;
	}
	m_buffer_offset = remaining;
}


bool AudioProcessor::Reset(int sample_rate, int num_channels)
{
	if (num_channels <= 0) {
		DEBUG("chromaprint::AudioProcessor::Reset() -- No audio channels.");
		return false;
	}
	if (sample_rate <= kMinSampleRate) {
		DEBUG("chromaprint::AudioProcessor::Reset() -- Sample rate less than "
              << kMinSampleRate << " (" << sample_rate << ").");
		return false;
	}
	m_buffer_offset = 0;
	if (m_resample_ctx) {
		av_resample_close(m_resample_ctx);
		m_resample_ctx = 0;
	}
	if (sample_rate != m_target_sample_rate) {
		m_resample_ctx = av_resample_init(
			m_target_sample_rate, sample_rate,
			kResampleFilterLength,
			kResamplePhaseShift,
			kResampleLinear,
			kResampleCutoff);
	}
	m_num_channels = num_channels;
	return true;
}

void AudioProcessor::Consume(const int16_t *input, int length)
{
	assert(length >= 0);
	assert(length % m_num_channels == 0);
	length /= m_num_channels;
	while (length > 0) {
		int consumed = Load(input, length); 
		input += consumed * m_num_channels;
		length -= consumed;
		if (m_buffer.size() == m_buffer_offset) {
			Resample();
			if (m_buffer.size() == m_buffer_offset) {
				DEBUG("chromaprint::AudioProcessor::Consume() -- Resampling failed?");
				return;
			}
		}
	}
}

void AudioProcessor::Flush()
{
	if (m_buffer_offset) {
		Resample();
	}
}

}; // namespace chromaprint
