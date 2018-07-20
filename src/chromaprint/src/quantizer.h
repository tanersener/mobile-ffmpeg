// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_QUANTIZER_H_
#define CHROMAPRINT_QUANTIZER_H_

#include <cassert>
#include <ostream>

namespace chromaprint {

class Quantizer {
public:
	Quantizer(double t0 = 0.0, double t1 = 0.0, double t2 = 0.0)
		: m_t0(t0), m_t1(t1), m_t2(t2)
	{
		assert(t0 <= t1 && t1 <= t2);
	}

	int Quantize(double value) const
	{
		if (value < m_t1) {
			if (value < m_t0) {
				return 0;
			}
			return 1;
		}
		else {
			if (value < m_t2) {
				return 2;
			}
			return 3;
		}
	}

	double t0() const { return m_t0; }
	void set_t0(double t) { m_t0 = t; }

	double t1() const { return m_t1; }
	void set_t1(double t) { m_t1 = t; }

	double t2() const { return m_t2; }
	void set_t2(double t) { m_t2 = t; }

private:
	double m_t0, m_t1, m_t2;
};

inline std::ostream &operator<<(std::ostream &stream, const Quantizer &q)
{
	stream << "Quantizer(" << q.t0() << ", " << q.t1() << ", " << q.t2() << ")";
	return stream;
}

}; // namespace chromaprint

#endif
