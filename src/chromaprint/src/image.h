// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_IMAGE_H_
#define CHROMAPRINT_IMAGE_H_

#include <vector>
#include <algorithm>
#include <cassert>

#ifdef NDEBUG
#define CHROMAPRINT_IMAGE_ROW_TYPE double *
#define CHROMAPRINT_IMAGE_ROW_TYPE_CAST(x, c) x
#else
#define CHROMAPRINT_IMAGE_ROW_TYPE ImageRow
#define CHROMAPRINT_IMAGE_ROW_TYPE_CAST(x, c) ImageRow(x, c)
#endif

namespace chromaprint {

class ImageRow
{
public:
	explicit ImageRow(double *data, int columns) : m_data(data), m_columns(columns)
	{
	}

	int NumColumns() const { return m_columns; }

	double &Column(int i)
	{
		assert(0 <= i && i < NumColumns());
		return m_data[i];
	}

	double &operator[](int i)
	{
		return Column(i);
	}

private:
	double *m_data;
	int m_columns;
};

class Image
{
public:
	explicit Image(int columns) : m_columns(columns)
	{
	}

	Image(int columns, int rows) : m_columns(columns), m_data(columns * rows)
	{
	}

	template<class Iterator>
	Image(int columns, Iterator first, Iterator last) : m_columns(columns), m_data(first, last)
	{
	}

	int NumColumns() const { return m_columns; }
	int NumRows() const { return m_data.size() / m_columns; }

	void AddRow(const std::vector<double> &row)
	{
		m_data.resize(m_data.size() + m_columns);
		std::copy(row.begin(), row.end(), m_data.end() - m_columns);
	}

	CHROMAPRINT_IMAGE_ROW_TYPE Row(int i)
	{
		assert(0 <= i && i < NumRows());
		return CHROMAPRINT_IMAGE_ROW_TYPE_CAST(&m_data[m_columns * i], m_columns);
	}

	CHROMAPRINT_IMAGE_ROW_TYPE operator[](int i)
	{
		return Row(i);
	}

private:
	int m_columns;
	std::vector<double> m_data;
};

}; // namespace chromaprint

#endif
