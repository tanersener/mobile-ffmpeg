// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_AUDIO_AUDIO_SLICER_H_
#define CHROMAPRINT_AUDIO_AUDIO_SLICER_H_

#include <cstddef>
#include <cstdint>
#include <cassert>
#include <vector>
#include "debug.h"

namespace chromaprint {

template <typename T>
class AudioSlicer {
public:
	AudioSlicer(size_t size, size_t increment)
		: m_size(size), m_increment(increment), m_buffer(size * 2) {
		assert(size >= increment);
		Reset();
	}

	size_t size() const {
		return m_size;
	}

	size_t increment() const {
		return m_increment;
	}

	void Reset() {
		m_buffer_begin = m_buffer_end = m_buffer.begin();
	}

	template <typename InputIt, typename ConsumerFunc>
	void Process(InputIt begin, InputIt end, ConsumerFunc consumer) {
		size_t size = std::distance(begin, end);
		size_t buffer_size = std::distance(m_buffer_begin, m_buffer_end);

		while (buffer_size > 0 && buffer_size + size >= m_size) {
			consumer(&(*m_buffer_begin), &(*m_buffer_end), begin, std::next(begin, m_size - buffer_size));
			if (buffer_size >= m_increment) {
				std::advance(m_buffer_begin, m_increment);
				buffer_size -= m_increment;
				const size_t available_buffer_size = std::distance(m_buffer_end, m_buffer.end());
				if (buffer_size + available_buffer_size < m_size) {
					const auto new_buffer_begin = m_buffer.begin();
					m_buffer_end = std::copy(m_buffer_begin, m_buffer_end, new_buffer_begin);
					m_buffer_begin = new_buffer_begin;
				}
			} else {
				m_buffer_begin = m_buffer_end = m_buffer.begin();
				std::advance(begin, m_increment - buffer_size);
				size -= m_increment - buffer_size;
				buffer_size = 0;
			}
		}

		if (buffer_size == 0) {
			while (size >= m_size) {
				consumer(begin, std::next(begin, m_size), end, end);
				std::advance(begin, m_increment);
				size -= m_increment;
			}
		}

		assert(buffer_size + size < m_size);
		m_buffer_end = std::copy(begin, end, m_buffer_end);
	}

private:
	size_t m_size;
	size_t m_increment;
	std::vector<T> m_buffer;
	typename std::vector<T>::iterator m_buffer_begin;
	typename std::vector<T>::iterator m_buffer_end;
};

}; // namespace chromaprint

#endif // CHROMAPRINT_AUDIO_AUDIO_SLICER_H_
