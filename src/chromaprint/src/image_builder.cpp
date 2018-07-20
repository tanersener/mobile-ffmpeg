// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#include <limits>
#include <cassert>
#include <cmath>
#include "image_builder.h"

namespace chromaprint {

ImageBuilder::ImageBuilder(Image *image)
	: m_image(image)
{
}

ImageBuilder::~ImageBuilder()
{
}

void ImageBuilder::Consume(std::vector<double> &features)
{
	assert(features.size() == (size_t)m_image->NumColumns());
	m_image->AddRow(features);
}

}; // namespace chromaprint
