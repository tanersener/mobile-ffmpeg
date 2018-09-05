// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_FINGERPRINTER_H_
#define CHROMAPRINT_FINGERPRINTER_H_

#include <stdint.h>
#include <vector>
#include "audio_consumer.h"

namespace chromaprint {

class FFT;
class Chroma;
class ChromaNormalizer;
class ChromaFilter;
class AudioProcessor;
class FingerprintCalculator;
class FingerprinterConfiguration;
class SilenceRemover;

class Fingerprinter : public AudioConsumer
{
public:
	Fingerprinter(FingerprinterConfiguration *config = 0);
	~Fingerprinter();

	/**
	 * Initialize the fingerprinting process.
	 */
	bool Start(int sample_rate, int num_channels);

	/**
	 * Process a block of raw audio data. Call this method as many times
	 * as you need. 
	 */
	void Consume(const int16_t *input, int length);

	/**
	 * Calculate the fingerprint based on the provided audio data.
	 */
	void Finish();

	//! Get the fingerprint generate from data up to this point.
	const std::vector<uint32_t> &GetFingerprint() const;

	//! Clear the generated fingerprint, but allow more audio to be processed.
	void ClearFingerprint();

	bool SetOption(const char *name, int value);

	const FingerprinterConfiguration *config() { return m_config; }

private:
	Chroma *m_chroma;
	ChromaNormalizer *m_chroma_normalizer;
	ChromaFilter *m_chroma_filter;
	FFT *m_fft;
	AudioProcessor *m_audio_processor;
	FingerprintCalculator *m_fingerprint_calculator;
	FingerprinterConfiguration *m_config;
	SilenceRemover *m_silence_remover;
};

}; // namespace chromaprint

#endif
