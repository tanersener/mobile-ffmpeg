// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#include <limits>
#include <cmath>
#include "fft_frame.h"
#include "utils.h"
#include "chroma.h"
#include "debug.h"

namespace chromaprint {

static const int NUM_BANDS = 12;

inline double FreqToOctave(double freq, double base = 440.0 / 16.0)
{
	return log(freq / base) / log(2.0);
}

Chroma::Chroma(int min_freq, int max_freq, int frame_size, int sample_rate, FeatureVectorConsumer *consumer)
	: m_interpolate(false),
	  m_notes(frame_size),
	  m_notes_frac(frame_size),
	  m_features(NUM_BANDS),
	  m_consumer(consumer)
{
	PrepareNotes(min_freq, max_freq, frame_size, sample_rate);
}

Chroma::~Chroma()
{
}

void Chroma::PrepareNotes(int min_freq, int max_freq, int frame_size, int sample_rate)
{
	m_min_index = std::max(1, FreqToIndex(min_freq, frame_size, sample_rate));
	m_max_index = std::min(frame_size / 2, FreqToIndex(max_freq, frame_size, sample_rate));
	for (int i = m_min_index; i < m_max_index; i++) {
		double freq = IndexToFreq(i, frame_size, sample_rate);
		double octave = FreqToOctave(freq);
		double note = NUM_BANDS * (octave - floor(octave)); 
		m_notes[i] = (char)note;
		m_notes_frac[i] = note - m_notes[i];
	}
}

void Chroma::Reset()
{
}

void Chroma::Consume(const FFTFrame &frame)
{
	fill(m_features.begin(), m_features.end(), 0.0);
	for (int i = m_min_index; i < m_max_index; i++) {
		int note = m_notes[i];
		double energy = frame[i];
		if (m_interpolate) {
			int note2 = note;
			double a = 1.0;
			if (m_notes_frac[i] < 0.5) {
				note2 = (note + NUM_BANDS - 1) % NUM_BANDS;
				a = 0.5 + m_notes_frac[i];
			}
			if (m_notes_frac[i] > 0.5) {
				note2 = (note + 1) % NUM_BANDS;
				a = 1.5 - m_notes_frac[i];
			}
			m_features[note] += energy * a; 
			m_features[note2] += energy * (1.0 - a); 
		}
		else {
			m_features[note] += energy; 
		}
	}
	m_consumer->Consume(m_features);
}

}; // namespace chromaprint
