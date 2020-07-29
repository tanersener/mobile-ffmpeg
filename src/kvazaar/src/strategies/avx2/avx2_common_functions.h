#ifndef AVX2_COMMON_FUNCTIONS_H
#define AVX2_COMMON_FUNCTIONS_H

#include <immintrin.h>

// The calling convention used by MSVC on 32-bit builds will essentially
// disallow functions to have more than 3 XMM/YMM parameters, because it
// will not provide more than 8-byte param alignment, and only the first
// three vector params will be carried in SIMD registers. Now the
// vectorcall convention could probably be problematic in globally visible
// funcitons, but likely not in static ones.
#if defined _MSC_VER && defined _WIN32 && !defined _WIN64
  #define FIX_W32 __vectorcall
#else
  #define FIX_W32
#endif

// Non-inline functions defined in this header are likely to trigger a
// warning for each module including this header that does NOT use them,
// at least on unix-ish platforms (GCC/Clang both on native Unix and MinGW).
// Tell 'em we actually want to do that, it's not an accident.
#if defined __GNUC__ || defined __clang__ || defined __MINGW32__ || defined __MINGW64__
  #define FIX_UNUSED __attribute__((unused))
#else
  #define FIX_UNUSED
#endif

#define FIX_NOINLINE FIX_W32 FIX_UNUSED

/*
 * Reorder coefficients from raster to scan order
 * Fun fact: Once upon a time, doing this in a loop looked like this:
 * for (int32_t n = 0; n < width * height; n++) {
 *   coef_reord[n] = coef[scan[n]];
 *   q_coef_reord[n] = q_coef[scan[n]];
 * }
 */
static INLINE void scanord_read_vector(const int16_t **__restrict coeffs, const uint32_t *__restrict scan, int8_t scan_mode, int32_t subpos, int32_t width, __m256i *result_vecs, const int n_bufs)
{
  // For vectorized reordering of coef and q_coef
  const __m128i low128_shuffle_masks[3] = {
    _mm_setr_epi8(10,11,  4, 5, 12,13,  0, 1,  6, 7, 14,15,  8, 9,  2, 3),
    _mm_setr_epi8( 0, 1,  2, 3,  4, 5,  6, 7,  8, 9, 10,11, 12,13, 14,15),
    _mm_setr_epi8( 4, 5,  6, 7,  0, 1,  2, 3, 12,13, 14,15,  8, 9, 10,11),
  };

  const __m128i blend_masks[3] = {
    _mm_setr_epi16( 0,  0,  0, -1,  0,  0, -1, -1),
    _mm_setr_epi16( 0,  0,  0,  0,  0,  0,  0,  0),
    _mm_setr_epi16( 0,  0, -1, -1,  0,  0, -1, -1),
  };

  const __m128i invec_rearr_masks_upper[3] = {
    _mm_setr_epi8( 0, 1,  8, 9,  2, 3,  6, 7, 10,11,  4, 5, 12,13, 14,15),
    _mm_setr_epi8( 0, 1,  2, 3,  4, 5,  6, 7,  8, 9, 10,11, 12,13, 14,15),
    _mm_setr_epi8( 0, 1,  8, 9,  4, 5, 12,13,  2, 3, 10,11,  6, 7, 14,15),
  };

  const __m128i invec_rearr_masks_lower[3] = {
    _mm_setr_epi8(12,13,  6, 7,  0, 1,  2, 3, 14,15,  4, 5,  8, 9, 10,11),
    _mm_setr_epi8( 0, 1,  2, 3,  4, 5,  6, 7,  8, 9, 10,11, 12,13, 14,15),
    _mm_setr_epi8( 4, 5, 12,13,  0, 1,  8, 9,  6, 7, 14,15,  2, 3, 10,11),
  };

  const size_t row_offsets[4] = {
    scan[subpos] + width * 0,
    scan[subpos] + width * 1,
    scan[subpos] + width * 2,
    scan[subpos] + width * 3,
  };

  for (int i = 0; i < n_bufs; i++) {
    const int16_t *__restrict coeff = coeffs[i];

    // NOTE: Upper means "higher in pixel order inside block", which implies
    // lower addresses (note the difference: HIGH and LOW vs UPPER and LOWER),
    // so upper 128b vector actually becomes the lower part of a 256-bit coeff
    // vector and lower vector the higher part!
    __m128d coeffs_d_upper;
    __m128d coeffs_d_lower;

    __m128i coeffs_upper;
    __m128i coeffs_lower;

    __m128i coeffs_rearr1_upper;
    __m128i coeffs_rearr1_lower;

    __m128i coeffs_rearr2_upper;
    __m128i coeffs_rearr2_lower;

    // Zeroing these is actually unnecessary, but the compiler will whine
    // about uninitialized values otherwise
    coeffs_d_upper = _mm_setzero_pd();
    coeffs_d_lower = _mm_setzero_pd();

    coeffs_d_upper = _mm_loadl_pd(coeffs_d_upper, (double *)(coeff + row_offsets[0]));
    coeffs_d_upper = _mm_loadh_pd(coeffs_d_upper, (double *)(coeff + row_offsets[1]));

    coeffs_d_lower = _mm_loadl_pd(coeffs_d_lower, (double *)(coeff + row_offsets[2]));
    coeffs_d_lower = _mm_loadh_pd(coeffs_d_lower, (double *)(coeff + row_offsets[3]));

    coeffs_upper   = _mm_castpd_si128(coeffs_d_upper);
    coeffs_lower   = _mm_castpd_si128(coeffs_d_lower);

    coeffs_lower   = _mm_shuffle_epi8(coeffs_lower, low128_shuffle_masks[scan_mode]);

    coeffs_rearr1_upper = _mm_blendv_epi8(coeffs_upper, coeffs_lower, blend_masks[scan_mode]);
    coeffs_rearr1_lower = _mm_blendv_epi8(coeffs_lower, coeffs_upper, blend_masks[scan_mode]);

    coeffs_rearr2_upper = _mm_shuffle_epi8(coeffs_rearr1_upper, invec_rearr_masks_upper[scan_mode]);
    coeffs_rearr2_lower = _mm_shuffle_epi8(coeffs_rearr1_lower, invec_rearr_masks_lower[scan_mode]);

    // The Intel Intrinsics Guide talks about _mm256_setr_m128i but my headers
    // lack such an instruction. What it does is essentially this anyway.
    result_vecs[i] = _mm256_inserti128_si256(_mm256_castsi128_si256(coeffs_rearr2_upper),
                                             coeffs_rearr2_lower,
                                             1);
  }
}

// If ints is completely zero, returns 16 in *first and -1 in *last
static INLINE void get_first_last_nz_int16(__m256i ints, int32_t *first, int32_t *last)
{
  // Note that nonzero_bytes will always have both bytes set for a set word
  // even if said word only had one of its bytes set, because we're doing 16
  // bit wide comparisons. No big deal, just shift results to the right by one
  // bit to have the results represent indexes of first set words, not bytes.
  // Another note, it has to use right shift instead of division to preserve
  // behavior on an all-zero vector (-1 / 2 == 0, but -1 >> 1 == -1)
  const __m256i zero = _mm256_setzero_si256();

  __m256i zeros = _mm256_cmpeq_epi16(ints, zero);
  uint32_t nonzero_bytes = ~((uint32_t)_mm256_movemask_epi8(zeros));
  *first = (    (int32_t)_tzcnt_u32(nonzero_bytes)) >> 1;
  *last = (31 - (int32_t)_lzcnt_u32(nonzero_bytes)) >> 1;
}

static int32_t FIX_NOINLINE hsum_8x32b(const __m256i v)
{
  __m256i sum1 = v;
  __m256i sum2 = _mm256_permute4x64_epi64(sum1, _MM_SHUFFLE(1, 0, 3, 2));
  __m256i sum3 = _mm256_add_epi32        (sum1, sum2);
  __m256i sum4 = _mm256_shuffle_epi32    (sum3, _MM_SHUFFLE(1, 0, 3, 2));
  __m256i sum5 = _mm256_add_epi32        (sum3, sum4);
  __m256i sum6 = _mm256_shuffle_epi32    (sum5, _MM_SHUFFLE(2, 3, 0, 1));
  __m256i sum7 = _mm256_add_epi32        (sum5, sum6);

  __m128i sum8 = _mm256_castsi256_si128  (sum7);
  int32_t sum9 = _mm_cvtsi128_si32       (sum8);
  return  sum9;
}

#endif
