#ifndef MISSING_INTEL_INTRINSICS_H_
#define MISSING_INTEL_INTRINSICS_H_

#include <immintrin.h>

// Old Visual Studio headers lack the bsrli variant
#ifndef _mm_bsrli_si128
  #define _mm_bsrli_si128(a, imm8) _mm_srli_si128((a), (imm8))
#endif

// GCC headers apparently won't have this at all.. sigh
#ifndef _andn_u32
  // VS2015 headers apparently won't have this at all.. sigh
  #ifdef __andn_u32
    #define _andn_u32(x, y) (__andn_u32((x), (y)))
  #else
    #define _andn_u32(x, y) ((~(x)) & (y))
  #endif // __andn_u32
#endif // _andn_u32

#endif
