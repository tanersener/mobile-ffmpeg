// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_SIMHASH_H_
#define CHROMAPRINT_SIMHASH_H_

#include <vector>
#include "utils.h"

namespace chromaprint {

uint32_t SimHash(const uint32_t *data, size_t size);

uint32_t SimHash(const std::vector<uint32_t> &data);

}; // namespace chromaprint

#endif
