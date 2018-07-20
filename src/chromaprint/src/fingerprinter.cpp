// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#include <string.h>
#include "fingerprinter.h"
#include "chroma.h"
#include "chroma_normalizer.h"
#include "chroma_filter.h"
#include "fft.h"
#include "audio_processor.h"
#include "image_builder.h"
#include "silence_remover.h"
#include "fingerprint_calculator.h"
#include "fingerprinter_configuration.h"
#include "classifier.h"
#include "utils.h"
#include "debug.h"

namespace chromaprint {

static const int MIN_FREQ = 28;
static const int MAX_FREQ = 3520;

Fingerprinter::Fingerprinter(FingerprinterConfiguration *config) {
	if (!config) {
		config = new FingerprinterConfigurationTest1();
	}
	m_fingerprint_calculator = new FingerprintCalculator(config->classifiers(), config->num_classifiers());
	m_chroma_normalizer = new ChromaNormalizer(m_fingerprint_calculator);
	m_chroma_filter = new ChromaFilter(config->filter_coefficients(), config->num_filter_coefficients(), m_chroma_normalizer);
	m_chroma = new Chroma(MIN_FREQ, MAX_FREQ, config->frame_size(), config->sample_rate(), m_chroma_filter);
	//m_chroma->set_interpolate(true);
	m_fft = new FFT(config->frame_size(), config->frame_overlap(), m_chroma);
	if (config->remove_silence()) {
		m_silence_remover = new SilenceRemover(m_fft);
		m_silence_remover->set_threshold(config->silence_threshold());
		m_audio_processor = new AudioProcessor(config->sample_rate(), m_silence_remover);
	}
	else {
		m_silence_remover = 0;
		m_audio_processor = new AudioProcessor(config->sample_rate(), m_fft);
	}
	m_config = config;
}

Fingerprinter::~Fingerprinter()
{
	delete m_audio_processor;
	if (m_silence_remover) {
		delete m_silence_remover;
	}
	delete m_fft;
	delete m_chroma;
	delete m_chroma_filter;
	delete m_chroma_normalizer;
	delete m_fingerprint_calculator;
	delete m_config;
}

bool Fingerprinter::SetOption(const char *name, int value)
{
	if (!strcmp(name, "silence_threshold")) {
		if (m_silence_remover) {
			m_silence_remover->set_threshold(value);
			return true;
		}
	}
	return false;
}

bool Fingerprinter::Start(int sample_rate, int num_channels)
{
	if (!m_audio_processor->Reset(sample_rate, num_channels)) {
		// FIXME save error message somewhere
		return false;
	}
	m_fft->Reset();
	m_chroma->Reset();
	m_chroma_filter->Reset();
	m_chroma_normalizer->Reset();
	m_fingerprint_calculator->Reset();
	return true;
}

void Fingerprinter::Consume(const int16_t *samples, int length)
{
	assert(length >= 0);
	m_audio_processor->Consume(samples, length);
}

void Fingerprinter::Finish()
{
	m_audio_processor->Flush();
}

const std::vector<uint32_t> &Fingerprinter::GetFingerprint() const {
	return m_fingerprint_calculator->GetFingerprint();
}

void Fingerprinter::ClearFingerprint() {
	m_fingerprint_calculator->ClearFingerprint();
}

}; // namespace chromaprint
