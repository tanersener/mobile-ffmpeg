// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_FINGERPRINTER_CONFIGURATION_H_
#define CHROMAPRINT_FINGERPRINTER_CONFIGURATION_H_

#include <algorithm>
#include "classifier.h"
#include "chromaprint.h"

namespace chromaprint {

static const int DEFAULT_SAMPLE_RATE = 11025;

class FingerprinterConfiguration
{
public:	

	FingerprinterConfiguration()
		: m_num_classifiers(0), m_classifiers(0), m_remove_silence(false), m_silence_threshold(0), m_frame_size(0), m_frame_overlap(0)
	{
	}

	int num_filter_coefficients() const
	{
		return m_num_filter_coefficients;
	}

	const double *filter_coefficients() const
	{
		return m_filter_coefficients;
	}

	void set_filter_coefficients(const double *filter_coefficients, int size)
	{
		m_filter_coefficients = filter_coefficients;
		m_num_filter_coefficients = size;
	}

	int num_classifiers() const
	{
		return m_num_classifiers;
	}

	const Classifier *classifiers() const
	{
		return m_classifiers;
	}

	int max_filter_width() const {
		return m_max_filter_width;
	}

	void set_classifiers(const Classifier *classifiers, int size)
	{
		m_classifiers = classifiers;
		m_num_classifiers = size;
		m_max_filter_width = 0;
		for (int i = 0; i < size; i++) {
			m_max_filter_width = std::max(m_max_filter_width, classifiers[i].filter().width());
		}
	}

	bool interpolate() const 
	{
		return m_interpolate;
	}

	void set_interpolate(bool value)
	{
		m_interpolate = value;
	}

	bool remove_silence() const 
	{
		return m_remove_silence;
	}

	void set_remove_silence(bool value)
	{
		m_remove_silence = value;
	}

	int silence_threshold() const
	{
		return m_silence_threshold;
	}

	void set_silence_threshold(int value)
	{
		m_silence_threshold = value;
	}

	int frame_size() const
	{
		return m_frame_size;
	}

	void set_frame_size(int value)
	{
		m_frame_size = value;
	}

	int frame_overlap() const
	{
		return m_frame_overlap;
	}

	void set_frame_overlap(int value)
	{
		m_frame_overlap = value;
	}

	int sample_rate() const {
		return DEFAULT_SAMPLE_RATE;
	}

	int item_duration() const {
		return m_frame_size - m_frame_overlap;
	}

	double item_duration_in_seconds() const {
		return item_duration() * 1.0 / sample_rate();
	}

	int delay() const {
		return ((m_num_filter_coefficients - 1) + (m_max_filter_width - 1)) * item_duration() + m_frame_overlap;
	}

	double delay_in_seconds() const {
		return delay() * 1.0 / sample_rate();
	}

private:
	int m_num_classifiers;
	int m_max_filter_width;
	const Classifier *m_classifiers;
	int m_num_filter_coefficients;
	const double *m_filter_coefficients;
	bool m_interpolate;
	bool m_remove_silence;
	int m_silence_threshold;
	int m_frame_size;
	int m_frame_overlap;
};

// Used for http://oxygene.sk/lukas/2010/07/introducing-chromaprint/
// Trained on a randomly selected test data
class FingerprinterConfigurationTest1 : public FingerprinterConfiguration
{
public:
	FingerprinterConfigurationTest1();
};

// Trained on 60k pairs based on eMusic samples (mp3)
class FingerprinterConfigurationTest2 : public FingerprinterConfiguration
{
public:
	FingerprinterConfigurationTest2();
};

// Trained on 60k pairs based on eMusic samples with interpolation enabled (mp3)
class FingerprinterConfigurationTest3 : public FingerprinterConfiguration
{
public:
	FingerprinterConfigurationTest3();
};

// Same as v2, but trims leading silence
class FingerprinterConfigurationTest4 : public FingerprinterConfigurationTest2
{
public:
	FingerprinterConfigurationTest4();
};

// Same as v2, but with 2x more precise sampling
class FingerprinterConfigurationTest5 : public FingerprinterConfigurationTest2
{
public:
	FingerprinterConfigurationTest5();
};

inline FingerprinterConfiguration *CreateFingerprinterConfiguration(int algorithm)
{
	switch (algorithm) {
	case CHROMAPRINT_ALGORITHM_TEST1:
		return new FingerprinterConfigurationTest1();
	case CHROMAPRINT_ALGORITHM_TEST2:
		return new FingerprinterConfigurationTest2();
	case CHROMAPRINT_ALGORITHM_TEST3:
		return new FingerprinterConfigurationTest3();
	case CHROMAPRINT_ALGORITHM_TEST4:
		return new FingerprinterConfigurationTest4();
	case CHROMAPRINT_ALGORITHM_TEST5:
		return new FingerprinterConfigurationTest5();
	}
	return 0;
}

}; // namespace chromaprint

#endif
