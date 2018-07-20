// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_AUDIO_FFMPEG_AUDIO_PROCESSOR_SWRESAMPLE_H_
#define CHROMAPRINT_AUDIO_FFMPEG_AUDIO_PROCESSOR_SWRESAMPLE_H_

extern "C" {
#include <libswresample/swresample.h>
}

namespace chromaprint {

class FFmpegAudioProcessor {
public:
	FFmpegAudioProcessor() {
		m_swr_ctx = swr_alloc();
	}

	~FFmpegAudioProcessor() {
		swr_free(&m_swr_ctx);
	}

	void SetCompatibleMode() {
		av_opt_set_int(m_swr_ctx, "resampler", SWR_ENGINE_SWR, 0);
		av_opt_set_int(m_swr_ctx, "filter_size", 16, 0);
		av_opt_set_int(m_swr_ctx, "phase_shift", 8, 0);
		av_opt_set_int(m_swr_ctx, "linear_interp", 1, 0);
		av_opt_set_double(m_swr_ctx, "cutoff", 0.8, 0);
	}

	void SetInputChannelLayout(int64_t channel_layout) {
		av_opt_set_int(m_swr_ctx, "icl", channel_layout, 0);
		av_opt_set_int(m_swr_ctx, "ich", av_get_channel_layout_nb_channels(channel_layout), 0);
	}

	void SetInputSampleFormat(AVSampleFormat sample_format) {
		av_opt_set_int(m_swr_ctx, "isf", sample_format, 0);
	}

	void SetInputSampleRate(int sample_rate) {
		av_opt_set_int(m_swr_ctx, "isr", sample_rate, 0);
	}

	void SetOutputChannelLayout(int64_t channel_layout) {
		av_opt_set_int(m_swr_ctx, "ocl", channel_layout, 0);
		av_opt_set_int(m_swr_ctx, "och", av_get_channel_layout_nb_channels(channel_layout), 0);
	}

	void SetOutputSampleFormat(AVSampleFormat sample_format) {
		av_opt_set_int(m_swr_ctx, "osf", sample_format, 0);
	}

	void SetOutputSampleRate(int sample_rate) {
		av_opt_set_int(m_swr_ctx, "osr", sample_rate, 0);
	}

	bool Init() {
		return swr_init(m_swr_ctx) >= 0;
	}

	int Convert(uint8_t **out, int out_count, const uint8_t **in, int in_count) {
		return swr_convert(m_swr_ctx, out, out_count, in, in_count);
	}

	int Flush(uint8_t **out, int out_count) {
		return swr_convert(m_swr_ctx, out, out_count, nullptr, 0);
	}

private:
	SwrContext *m_swr_ctx = nullptr;
};

}; // namespace chromaprint

#endif
