// Copyright (C) 2016  Lukas Lalinsky
// Distributed under the MIT license, see the LICENSE file for details.

#include "base64.h"
#include <cassert>

namespace chromaprint {

void Base64Encode(const std::string &src, std::string &dest)
{
	dest.resize(GetBase64EncodedSize(src.size()));
	const auto end = Base64Encode(src.cbegin(), src.cend(), dest.begin());
	assert(dest.end() == end);
}

std::string Base64Encode(const std::string &src)
{
	std::string dest;
	Base64Encode(src, dest);
	return dest;
}

void Base64Decode(const std::string &src, std::string &dest)
{
	dest.resize(GetBase64DecodedSize(src.size()));
	const auto end = Base64Decode(src.cbegin(), src.cend(), dest.begin());
	assert(dest.end() == end);
}

std::string Base64Decode(const std::string &src)
{
	std::string dest;
	Base64Decode(src, dest);
	return dest;
}

}; // namespace chromaprint
