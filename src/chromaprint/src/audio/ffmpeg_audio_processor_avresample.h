// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_AUDIO_FFMPEG_AUDIO_PROCESSOR_AVRESAMPLE_H_
#define CHROMAPRINT_AUDIO_FFMPEG_AUDIO_PROCESSOR_AVRESAMPLE_H_

extern "C" {
#include <libavresample/avresample.h>
}

namespace chromaprint {

class FFmpegAudioProcessor {
public:
	FFmpegAudioProcessor() {
		m_resample_ctx = avresample_alloc_context();
	}

	~FFmpegAudioProcessor() {
		avresample_free(&m_resample_ctx);
	}

	void SetCompatibleMode() {
		av_opt_set_int(m_resample_ctx, "filter_size", 16, 0);
		av_opt_set_int(m_resample_ctx, "phase_shift", 8, 0);
		av_opt_set_int(m_resample_ctx, "linear_interp", 1, 0);
		av_opt_set_double(m_resample_ctx, "cutoff", 0.8, 0);
	}

	void SetInputChannelLayout(int64_t channel_layout) {
		av_opt_set_int(m_resample_ctx, "in_channel_layout", channel_layout, 0);
	}

	void SetInputSampleFormat(AVSampleFormat sample_format) {
		av_opt_set_int(m_resample_ctx, "in_sample_fmt", sample_format, 0);
	}

	void SetInputSampleRate(int sample_rate) {
		av_opt_set_int(m_resample_ctx, "in_sample_rate", sample_rate, 0);
	}

	void SetOutputChannelLayout(int64_t channel_layout) {
		av_opt_set_int(m_resample_ctx, "out_channel_layout", channel_layout, 0);
	}

	void SetOutputSampleFormat(AVSampleFormat sample_format) {
		av_opt_set_int(m_resample_ctx, "out_sample_fmt", sample_format, 0);
	}

	void SetOutputSampleRate(int sample_rate) {
		av_opt_set_int(m_resample_ctx, "out_sample_fmt", sample_rate, 0);
	}

	int Init() {
		return avresample_open(m_resample_ctx);
	}

	int Convert(uint8_t **out, int out_count, const uint8_t **in, int in_count) {
		return avresample_convert(m_resample_ctx, out, 0, out_count, (uint8_t **) in, 0, in_count);
	}

	int Flush(uint8_t **out, int out_count) {
		return avresample_read(m_resample_ctx, out, out_count);
	}

private:
	AVAudioResampleContext *m_resample_ctx = nullptr;
};

}; // namespace chromaprint

#endif
