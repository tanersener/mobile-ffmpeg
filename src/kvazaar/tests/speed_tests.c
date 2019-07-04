/*****************************************************************************
 * This file is part of Kvazaar HEVC encoder.
 *
 * Copyright (C) 2013-2015 Tampere University of Technology and others (see
 * COPYING file).
 *
 * Kvazaar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1 as
 * published by the Free Software Foundation.
 *
 * Kvazaar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Kvazaar.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/
 
#include "greatest/greatest.h"

#include "test_strategies.h"

#include "src/image.h"
#include "src/threads.h"

#include <math.h>
#include <stdlib.h>


//////////////////////////////////////////////////////////////////////////
// MACROS
#define NUM_TESTS 113
#define NUM_CHUNKS 36
#define LCU_MAX_LOG_W 6
#define LCU_MIN_LOG_W 2

// Time per tested function, in seconds.
#define TIME_PER_TEST 1.0

//////////////////////////////////////////////////////////////////////////
// GLOBALS
static kvz_pixel * bufs[NUM_TESTS]; // SIMD aligned pointers.
static kvz_pixel * actual_bufs[NUM_TESTS]; // pointers returned by malloc.

#define WIDTH_4K 3840 
#define HEIGHT_4K 2160

static struct test_env_t {
  int width;
  int height;
  void * tested_func;
  const strategy_t * strategy;
  char msg[1024];
  
  kvz_picture *inter_a;
  kvz_picture *inter_b;
} test_env;


//////////////////////////////////////////////////////////////////////////
// SETUP, TEARDOWN AND HELPER FUNCTIONS
static void init_gradient(int x_px, int y_px, int width, int slope, kvz_pixel *buf)
{
  for (int y = 0; y < width; ++y) {
    for (int x = 0; x < width; ++x) {
      int diff_x = x_px - x;
      int diff_y = y_px - y;
      int val = slope * sqrt(diff_x * diff_x + diff_y * diff_y) + 0.5;
      buf[y * width + x] = CLIP(0, 255, val);
    }
  }
}


static void setup_tests()
{
  for (int test = 0; test < NUM_TESTS; ++test) {
    unsigned size = NUM_CHUNKS * 64 * 64;
    
    actual_bufs[test] = malloc(size * sizeof(kvz_pixel) + SIMD_ALIGNMENT);
    bufs[test] = ALIGNED_POINTER(actual_bufs[test], SIMD_ALIGNMENT);
  }

  for (int test = 0; test < NUM_TESTS; ++test) {
    for (int chunk = 0; chunk < NUM_CHUNKS; ++chunk) {
      const int width = 64;
      int x = (test + chunk) % width;
      int y = (test + chunk) / width;
      init_gradient(width - x, y, width, 255 / width, &bufs[test][chunk * 64*64]);
    }
  }

  test_env.inter_a = kvz_image_alloc(KVZ_CSP_420, WIDTH_4K, HEIGHT_4K);
  test_env.inter_b = kvz_image_alloc(KVZ_CSP_420, WIDTH_4K, HEIGHT_4K);
  for (unsigned i = 0; i < WIDTH_4K * HEIGHT_4K; ++i) {
    kvz_pixel pattern1 = ((i*i >> 10) % 255) >> 2;
    kvz_pixel pattern2 = ((i*i >> 15) % 255) >> 2;
    kvz_pixel gradient = (i >> 12) + i;
    test_env.inter_a->y[i] = (pattern1 + gradient) % PIXEL_MAX;
    test_env.inter_b->y[i] = (pattern2 + gradient) % PIXEL_MAX;
  }
}

static void tear_down_tests()
{
  for (int test = 0; test < NUM_TESTS; ++test) {
    free(actual_bufs[test]);
  }
  kvz_image_free(test_env.inter_a);
  kvz_image_free(test_env.inter_b);
}

//////////////////////////////////////////////////////////////////////////
// TESTS

TEST test_intra_speed(const int width)
{
  const int size = width * width;
  uint64_t call_cnt = 0;
  KVZ_CLOCK_T clock_now;
  KVZ_GET_TIME(&clock_now);
  double test_end = KVZ_CLOCK_T_AS_DOUBLE(clock_now) + TIME_PER_TEST;

  // Loop until time allocated for test has passed.
  for (unsigned i = 0; 
      test_end > KVZ_CLOCK_T_AS_DOUBLE(clock_now);
      ++i)
  {
    int test = i % NUM_TESTS;
    uint64_t sum = 0;
    for (int offset = 0; offset < NUM_CHUNKS * 64 * 64; offset += NUM_CHUNKS * size) {
      // Compare the first chunk against the 35 other chunks to simulate real usage.
      kvz_pixel * buf1 = &bufs[test][offset];
      for (int chunk = 1; chunk < NUM_CHUNKS; ++chunk) {
        kvz_pixel * buf2 = &bufs[test][chunk * size + offset];

        cost_pixel_nxn_func *tested_func = test_env.tested_func;
        sum += tested_func(buf1, buf2);
        ++call_cnt;
      }
    }

    ASSERT(sum > 0);
    KVZ_GET_TIME(&clock_now)
  }

  double test_time = TIME_PER_TEST + KVZ_CLOCK_T_AS_DOUBLE(clock_now) - test_end;
  sprintf(test_env.msg, "%.3fM x %s:%s",
    (double)call_cnt / 1000000.0 / test_time,
    test_env.strategy->type,
    test_env.strategy->strategy_name);
  PASSm(test_env.msg);
}


TEST test_intra_dual_speed(const int width)
{
  const int size = width * width;
  uint64_t call_cnt = 0;
  KVZ_CLOCK_T clock_now;
  KVZ_GET_TIME(&clock_now);
  double test_end = KVZ_CLOCK_T_AS_DOUBLE(clock_now) + TIME_PER_TEST;

  // Loop until time allocated for test has passed.
  for (unsigned i = 0;
    test_end > KVZ_CLOCK_T_AS_DOUBLE(clock_now);
    ++i)
  {
    int test = i % NUM_TESTS;
    uint64_t sum = 0;
    for (int offset = 0; offset < NUM_CHUNKS * 64 * 64; offset += NUM_CHUNKS * size) {
      // Compare the first chunk against the 35 other chunks to simulate real usage.
      kvz_pixel * buf1 = &bufs[test][offset];
      for (int chunk = 0; chunk < NUM_CHUNKS; chunk += 2) {
        cost_pixel_nxn_multi_func *tested_func = test_env.tested_func;
        const kvz_pixel *buf_pair[2] = { &bufs[test][chunk * size + offset], &bufs[test][(chunk + 1) * size + offset] };
        unsigned costs[2] = { 0, 0 };
        tested_func((pred_buffer)buf_pair, buf1, 2, costs);
        sum += costs[0] + costs[1];
        ++call_cnt;
      }
    }

    ASSERT(sum > 0);
    KVZ_GET_TIME(&clock_now)
  }

  double test_time = TIME_PER_TEST + KVZ_CLOCK_T_AS_DOUBLE(clock_now) - test_end;
  sprintf(test_env.msg, "%.3fM x %s:%s",
    (double)call_cnt / 1000000.0 / test_time,
    test_env.strategy->type,
    test_env.strategy->strategy_name);
  PASSm(test_env.msg);
}


TEST test_inter_speed(const int width, const int height)
{
  unsigned call_cnt = 0;
  KVZ_CLOCK_T clock_now;
  KVZ_GET_TIME(&clock_now);
  double test_end = KVZ_CLOCK_T_AS_DOUBLE(clock_now) + TIME_PER_TEST;

  const vector2d_t dims_lcu = { WIDTH_4K / 64 - 2, HEIGHT_4K / 64 - 2 };
  const int step = 3;
  const int range = 2 * step;

  // Loop until time allocated for test has passed.
  for (uint64_t i = 0;
      test_end > KVZ_CLOCK_T_AS_DOUBLE(clock_now);
      ++i)
  {
    // Do a sparse full search on the first CU of every LCU.
    
    uint64_t sum = 0;

    // Go through the non-edge LCU's in raster scan order.
    const vector2d_t lcu = {
      1 + i % dims_lcu.x,
      1 + (i / dims_lcu.y) % dims_lcu.y,
    };

    vector2d_t mv;
    for (mv.y = -range; mv.y <= range; mv.y += step) {
      for (mv.x = -range; mv.x <= range; mv.x += step) {
        reg_sad_func *tested_func = test_env.tested_func;

        int lcu_index = lcu.y * 64 * WIDTH_4K + lcu.x * 64;
        int mv_index = mv.y * WIDTH_4K + mv.x;
        kvz_pixel *buf1 = &test_env.inter_a->y[lcu_index];
        kvz_pixel *buf2 = &test_env.inter_a->y[lcu_index + mv_index];

        sum += tested_func(buf1, buf2, width, height, WIDTH_4K, WIDTH_4K);
        ++call_cnt;
      }
    }

    ASSERT(sum > 0);
    KVZ_GET_TIME(&clock_now)
  }

  double test_time = TIME_PER_TEST + KVZ_CLOCK_T_AS_DOUBLE(clock_now) - test_end;
  sprintf(test_env.msg, "%.3fM x %s(%ix%i):%s",
    (double)call_cnt / 1000000.0 / test_time,
    test_env.strategy->type,
    width,
    height,
    test_env.strategy->strategy_name);
  PASSm(test_env.msg);
}


TEST dct_speed(const int width)
{
  const int size = width * width;
  uint64_t call_cnt = 0;
  dct_func * tested_func = test_env.strategy->fptr;

  KVZ_CLOCK_T clock_now;
  KVZ_GET_TIME(&clock_now);
  double test_end = KVZ_CLOCK_T_AS_DOUBLE(clock_now) + TIME_PER_TEST;

  int16_t _tmp_residual[32 * 32 + SIMD_ALIGNMENT];
  int16_t _tmp_coeffs[32 * 32 + SIMD_ALIGNMENT];
  int16_t *tmp_residual = ALIGNED_POINTER(_tmp_residual, SIMD_ALIGNMENT);
  int16_t *tmp_coeffs = ALIGNED_POINTER(_tmp_coeffs, SIMD_ALIGNMENT);
  
  // Loop until time allocated for test has passed.
  for (unsigned i = 0;
    test_end > KVZ_CLOCK_T_AS_DOUBLE(clock_now);
    ++i)
  {
    int test = i % NUM_TESTS;
    uint64_t sum = 0;
    for (int offset = 0; offset < NUM_CHUNKS * 64 * 64; offset += NUM_CHUNKS * size) {
      // Compare the first chunk against the 35 other chunks to simulate real usage.
      for (int chunk = 0; chunk < NUM_CHUNKS; ++chunk) {
        kvz_pixel * buf1 = &bufs[test][offset];
        kvz_pixel * buf2 = &bufs[test][chunk * size + offset];
        for (int p = 0; p < size; ++p) {
          tmp_residual[p] = (int16_t)(buf1[p] - buf2[p]);
        }

        tested_func(8, tmp_residual, tmp_coeffs);
        ++call_cnt;
        sum += tmp_coeffs[0];
      }
    }

    ASSERT(sum > 0);
    KVZ_GET_TIME(&clock_now)
  }
  
  double test_time = TIME_PER_TEST + KVZ_CLOCK_T_AS_DOUBLE(clock_now) - test_end;
  sprintf(test_env.msg, "%.3fM x %s:%s",
    (double)call_cnt / 1000000.0 / test_time,
    test_env.strategy->type,
    test_env.strategy->strategy_name);
  PASSm(test_env.msg);
}


TEST intra_sad(void)
{
  return test_intra_speed(test_env.width);
}


TEST intra_sad_dual(void)
{
  return test_intra_dual_speed(test_env.width);
}


TEST intra_satd(void)
{
  return test_intra_speed(test_env.width);
}


TEST intra_satd_dual(void)
{
  return test_intra_dual_speed(test_env.width);
}


TEST inter_sad(void)
{
  return test_inter_speed(test_env.width, test_env.height);
}


TEST fdct(void)
{
  return dct_speed(test_env.width);
}


TEST idct(void)
{
  return dct_speed(test_env.width);
}



//////////////////////////////////////////////////////////////////////////
// TEST FIXTURES
SUITE(speed_tests)
{
  //SET_SETUP(sad_setup);
  //SET_TEARDOWN(sad_teardown);

  setup_tests();

  // Loop through all strategies picking out the intra sad ones and run
  // selectec strategies though all tests
  for (volatile unsigned i = 0; i < strategies.count; ++i) {
    const strategy_t * strategy = &strategies.strategies[i];

    // Select buffer width according to function name.
    if (strstr(strategy->type, "_4x4")) {
      test_env.width = 4;
      test_env.height = 4;
    } else if (strstr(strategy->type, "_8x8")) {
      test_env.width = 8;
      test_env.height = 8;
    } else if (strstr(strategy->type, "_16x16")) {
      test_env.width = 16;
      test_env.height = 16;
    } else if (strstr(strategy->type, "_32x32")) {
      test_env.width = 32;
      test_env.height = 32;
    } else if (strstr(strategy->type, "_64x64")) {
      test_env.width = 64;
      test_env.height = 64;
    } else {
      test_env.width = 0;
      test_env.height = 0;
    }

    test_env.tested_func = strategies.strategies[i].fptr;
    test_env.strategy = strategy;

    // Call different tests depending on type of function.
    // This allows for selecting a subset of tests with -t parameter.
    if (strncmp(strategy->type, "satd_", 5) == 0 && strcmp(strategy->type, "satd_any_size") != 0) {
      if (strlen(strategy->type) <= 10) {
        RUN_TEST(intra_satd);
      } else if (strstr(strategy->type, "_dual")) {
        RUN_TEST(intra_satd_dual);
      }
    } else if (strncmp(strategy->type, "sad_", 4) == 0) {
      if (strlen(strategy->type) <= 9) {
        RUN_TEST(intra_sad);
      } else if (strstr(strategy->type, "_dual")) {
        RUN_TEST(intra_sad_dual);
      }
    } else if (strcmp(strategy->type, "reg_sad") == 0) {
      static const vector2d_t tested_dims[] = { 
          { 8, 8 }, { 16, 16 }, { 32, 32 }, { 64, 64 },
          { 64, 63 }, { 1, 1 }
      };

      int num_tested_dims = sizeof(tested_dims) / sizeof(*tested_dims);

      // Call reg_sad with all the sizes it is actually called with.
      for (volatile int dim_i = 0; dim_i < num_tested_dims; ++dim_i) {
        test_env.width = tested_dims[dim_i].x;
        test_env.height = tested_dims[dim_i].y;
        RUN_TEST(inter_sad);
      }

    } else if (strncmp(strategy->type, "dct_", 4) == 0 ||
               strcmp(strategy->type, "fast_forward_dst_4x4") == 0)
    {
      RUN_TEST(fdct);
    } else if (strncmp(strategy->type, "idct_", 4) == 0 ||
               strcmp(strategy->type, "fast_inverse_dst_4x4") == 0)
    {
      RUN_TEST(idct);
    }
  }

  tear_down_tests();
}
