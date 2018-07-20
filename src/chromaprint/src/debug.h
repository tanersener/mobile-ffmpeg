// Copyright (C) 2010-2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#ifndef CHROMAPRINT_DEBUG_H_
#define CHROMAPRINT_DEBUG_H_

#ifdef NDEBUG
#include <ostream>
#else
#include <iostream>
#endif

namespace chromaprint {

#ifdef NDEBUG
#define DEBUG(x)
#else
#define DEBUG(x) std::cerr << x << std::endl
#endif

}; // namespace chromaprint

#endif
