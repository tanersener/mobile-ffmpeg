// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#include <algorithm>
#include <numeric>
#include <iostream>
#include "fingerprint_matcher.h"
#include "fingerprinter_configuration.h"
#include "utils.h"
#include "utils/gaussian_filter.h"
#include "utils/gradient.h"
#include "debug.h"

namespace chromaprint {

/* fingerprint matcher settings */
#define ACOUSTID_MAX_BIT_ERROR 2
#define ACOUSTID_MAX_ALIGN_OFFSET 120
#define ACOUSTID_QUERY_START 80
#define ACOUSTID_QUERY_LENGTH 120
#define ACOUSTID_QUERY_BITS 28
#define ACOUSTID_QUERY_MASK (((1<<ACOUSTID_QUERY_BITS)-1)<<(32-ACOUSTID_QUERY_BITS))
#define ACOUSTID_QUERY_STRIP(x) ((x) & ACOUSTID_QUERY_MASK)

#define ALIGN_BITS 12
#define ALIGN_MASK ((1 << ALIGN_BITS) - 1)
#define ALIGN_STRIP(x) ((uint32_t)(x) >> (32 - ALIGN_BITS))

#define UNIQ_BITS 16
#define UNIQ_MASK ((1 << MATCH_BITS) - 1)
#define UNIQ_STRIP(x) ((uint32_t)(x) >> (32 - MATCH_BITS))

FingerprintMatcher::FingerprintMatcher(FingerprinterConfiguration *config)
	: m_config(config)
{
}

double FingerprintMatcher::GetHashTime(size_t i) const {
	return m_config->item_duration_in_seconds() * i;
}

double FingerprintMatcher::GetHashDuration(size_t i) const {
	return GetHashTime(i) + m_config->delay_in_seconds();
}

bool FingerprintMatcher::Match(const std::vector<uint32_t> &fp1, const std::vector<uint32_t> &fp2)
{
	return Match(fp1.data(), fp1.size(), fp2.data(), fp2.size());
}

bool FingerprintMatcher::Match(const uint32_t fp1_data[], size_t fp1_size, const uint32_t fp2_data[], size_t fp2_size)
{
	const uint32_t hash_shift = 32 - ALIGN_BITS;
	const uint32_t hash_mask = ((1u << ALIGN_BITS) - 1) << hash_shift;
	const uint32_t offset_mask = (1u << (32 - ALIGN_BITS - 1)) - 1;
	const uint32_t source_mask = 1u << (32 - ALIGN_BITS - 1);

	if (fp1_size + 1 >= offset_mask) {
		DEBUG("chromaprint::FingerprintMatcher::Match() -- Fingerprint 1 too long.");
		return false;
	}
	if (fp2_size + 1 >= offset_mask) {
		DEBUG("chromaprint::FingerprintMatcher::Match() -- Fingerprint 2 too long.");
		return false;
	}

	m_offsets.clear();
	m_offsets.reserve(fp1_size + fp2_size);
	for (size_t i = 0; i < fp1_size; i++) {
		m_offsets.push_back((ALIGN_STRIP(fp1_data[i]) << hash_shift) | uint32_t(i & offset_mask));
	}
	for (size_t i = 0; i < fp2_size; i++) {
		m_offsets.push_back((ALIGN_STRIP(fp2_data[i]) << hash_shift) | uint32_t(i & offset_mask) | source_mask);
	}
	std::sort(m_offsets.begin(), m_offsets.end());

	m_histogram.assign(fp1_size + fp2_size, 0);
	for (auto it = m_offsets.cbegin(); it != m_offsets.cend(); ++it) {
		const uint32_t hash = (*it) & hash_mask;
		const uint32_t offset1 = (*it) & offset_mask;
		const uint32_t source1 = (*it) & source_mask;
		if (source1 != 0) {
			// if we got hash from fp2, it means there is no hash from fp1,
			// because if there was, it would be first
			continue;
		}
		auto it2 = it;
		while (++it2 != m_offsets.end()) {
			const uint32_t hash2 = (*it2) & hash_mask;
			if (hash != hash2) {
				break;
			}
			const uint32_t offset2 = (*it2) & offset_mask;
			const uint32_t source2 = (*it2) & source_mask;
			if (source2 != 0) {
				const size_t offset_diff = offset1 + fp2_size - offset2;
				m_histogram[offset_diff] += 1;
			}
		}
	}

	m_best_alignments.clear();
	const auto histogram_size = m_histogram.size();
	for (size_t i = 0; i < histogram_size; i++) {
		const uint32_t count = m_histogram[i];
		if (m_histogram[i] > 1) {
			const bool is_peak_left = (i > 0) ? m_histogram[i - 1] <= count : true;
			const bool is_peak_right = (i < histogram_size - 1) ? m_histogram[i + 1] <= count : true;
			if (is_peak_left && is_peak_right) {
				m_best_alignments.push_back(std::make_pair(count, i));
			}
		}
	}
	std::sort(m_best_alignments.rbegin(), m_best_alignments.rend());

	m_segments.clear();

	for (const auto &item : m_best_alignments) {
		const int offset_diff = item.second - fp2_size;

		const size_t offset1 = offset_diff > 0 ? offset_diff : 0;
		const size_t offset2 = offset_diff < 0 ? -offset_diff : 0;

		auto it1 = fp1_data + offset1;
		auto it2 = fp2_data + offset2;

		const auto size = std::min(fp1_size - offset1, fp2_size - offset2);
		std::vector<float> bit_counts(size);
		for (size_t i = 0; i < size; i++) {
			bit_counts[i] = HammingDistance(*it1++, *it2++) + rand() * (0.001f / RAND_MAX);
		}

		std::vector<float> orig_bit_counts = bit_counts;
		std::vector<float> smoothed_bit_counts;
		GaussianFilter(bit_counts, smoothed_bit_counts, 8.0, 3);

		std::vector<float> gradient(size);
		Gradient(smoothed_bit_counts.begin(), smoothed_bit_counts.end(), gradient.begin());

		for (size_t i = 0; i < size; i++) {
			gradient[i] = std::abs(gradient[i]);
		}

		std::vector<size_t> gradient_peaks;
		for (size_t i = 0; i < size; i++) {
			const auto gi = gradient[i];
			if (i > 0 && i < size - 1 && gi > 0.15 && gi >= gradient[i - 1] && gi >= gradient[i + 1]) {
				if (gradient_peaks.empty() || gradient_peaks.back() + 1 < i) {
					gradient_peaks.push_back(i);
				}
			}
		}
		gradient_peaks.push_back(size);

		size_t begin = 0;
		for (size_t end : gradient_peaks) {
			const auto duration = end - begin;
			const auto score = std::accumulate(orig_bit_counts.begin() + begin, orig_bit_counts.begin() + end, 0.0) / duration;
			if (score < m_match_threshold) {
				bool added = false;
				if (!m_segments.empty()) {
					auto &s1 = m_segments.back();
					if (std::abs(s1.score - score) < 0.7) {
						s1 = s1.merged(Segment(offset1 + begin, offset2 + begin, duration, score));
						added = true;
					}
				}
				if (!added) {
					m_segments.emplace_back(offset1 + begin, offset2 + begin, duration, score);
				}
			}
			begin = end;
		}

		// TODO try to merge segments from multiple offsets

		break;
	}
	
	return true;
}

}; // namespace chromaprint
