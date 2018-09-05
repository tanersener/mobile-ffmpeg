// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_AUDIO_PROCESSOR_H_
#define CHROMAPRINT_AUDIO_PROCESSOR_H_

#include "utils.h"
#include "audio_consumer.h"
#include <vector>

struct AVResampleContext;

namespace chromaprint
{

	class AudioProcessor : public AudioConsumer
	{
	public:
		AudioProcessor(int sample_rate, AudioConsumer *consumer);
		virtual ~AudioProcessor();

		int target_sample_rate() const
		{
			return m_target_sample_rate;
		}

		void set_target_sample_rate(int sample_rate)
		{
			m_target_sample_rate = sample_rate;
		}

		AudioConsumer *consumer() const
		{
			return m_consumer;
		}

		void set_consumer(AudioConsumer *consumer)
		{
			m_consumer = consumer;
		}

		//! Prepare for a new audio stream
		bool Reset(int sample_rate, int num_channels);

		//! Process a chunk of data from the audio stream
		void Consume(const int16_t *input, int length);

		//! Process any buffered input that was not processed before and clear buffers
		void Flush();

	private:
		CHROMAPRINT_DISABLE_COPY(AudioProcessor);

		int Load(const int16_t *input, int length);
		void LoadMono(const int16_t *input, int length);
		void LoadStereo(const int16_t *input, int length);
		void LoadMultiChannel(const int16_t *input, int length);
		void Resample();

		std::vector<int16_t> m_buffer;
		size_t m_buffer_offset;
		std::vector<int16_t> m_resample_buffer;
		int m_target_sample_rate;
		int m_num_channels;
		AudioConsumer *m_consumer;
		struct AVResampleContext *m_resample_ctx;
	};

};

#endif

