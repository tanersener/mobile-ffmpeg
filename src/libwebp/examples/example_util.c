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

#include "./example_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//------------------------------------------------------------------------------
// String parsing

uint32_t ExUtilGetUInt(const char* const v, int base, int* const error) {
  char* end = NULL;
  const uint32_t n = (v != NULL) ? (uint32_t)strtoul(v, &end, base) : 0u;
  if (end == v && error != NULL && !*error) {
    *error = 1;
    fprintf(stderr, "Error! '%s' is not an integer.\n",
            (v != NULL) ? v : "(null)");
  }
  return n;
}

int ExUtilGetInt(const char* const v, int base, int* const error) {
  return (int)ExUtilGetUInt(v, base, error);
}

int ExUtilGetInts(const char* v, int base, int max_output, int output[]) {
  int n, error = 0;
  for (n = 0; v != NULL && n < max_output; ++n) {
    const int value = ExUtilGetInt(v, base, &error);
    if (error) return -1;
    output[n] = value;
    v = strchr(v, ',');
    if (v != NULL) ++v;   // skip over the trailing ','
  }
  return n;
}

float ExUtilGetFloat(const char* const v, int* const error) {
  char* end = NULL;
  const float f = (v != NULL) ? (float)strtod(v, &end) : 0.f;
  if (end == v && error != NULL && !*error) {
    *error = 1;
    fprintf(stderr, "Error! '%s' is not a floating point number.\n",
            (v != NULL) ? v : "(null)");
  }
  return f;
}
