// Copyright 2012 Google Inc. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the COPYING file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS. All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.
// -----------------------------------------------------------------------------
//
//  Utility functions used by the example programs.
//

#ifndef WEBP_EXAMPLES_EXAMPLE_UTIL_H_
#define WEBP_EXAMPLES_EXAMPLE_UTIL_H_

#include "webp/types.h"

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
// String parsing

// Parses 'v' using strto(ul|l|d)(). If error is non-NULL, '*error' is set to
// true on failure while on success it is left unmodified to allow chaining of
// calls. An error is only printed on the first occurrence.
uint32_t ExUtilGetUInt(const char* const v, int base, int* const error);
int ExUtilGetInt(const char* const v, int base, int* const error);
float ExUtilGetFloat(const char* const v, int* const error);

// This variant of ExUtilGetInt() will parse multiple integers from a
// comma-separated list. Up to 'max_output' integers are parsed.
// The result is placed in the output[] array, and the number of integers
// actually parsed is returned, or -1 if an error occurred.
int ExUtilGetInts(const char* v, int base, int max_output, int output[]);

#ifdef __cplusplus
}    // extern "C"
#endif

#endif  // WEBP_EXAMPLES_EXAMPLE_UTIL_H_
