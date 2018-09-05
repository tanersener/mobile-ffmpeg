// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_IMAGE_BUILDER_H_
#define CHROMAPRINT_IMAGE_BUILDER_H_

#include <vector>
#include "utils.h"
#include "image.h"
#include "feature_vector_consumer.h"

namespace chromaprint {

class ImageBuilder : public FeatureVectorConsumer {
public:
	ImageBuilder(Image *image = 0);
	~ImageBuilder();

	void Reset(Image *image) 	{
		set_image(image);
	}

	void Consume(std::vector<double> &features);

	Image *image() const {
		return m_image;
	}

	void set_image(Image *image) {
		m_image = image;
	}

private:
	CHROMAPRINT_DISABLE_COPY(ImageBuilder);

	Image *m_image;
};

}; // namespace chromaprint

#endif
