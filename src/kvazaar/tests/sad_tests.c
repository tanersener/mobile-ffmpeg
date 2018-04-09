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

#include <string.h>


//////////////////////////////////////////////////////////////////////////
// EXTERNAL FUNCTIONS

//////////////////////////////////////////////////////////////////////////
// DEFINES
#define TEST_SAD(X, Y) kvz_image_calc_sad(g_pic, g_ref, 0, 0, (X), (Y), 8, 8)

//////////////////////////////////////////////////////////////////////////
// GLOBALS
static const kvz_pixel ref_data[64] = {
  1,2,2,2,2,2,2,3,
  4,5,5,5,5,5,5,6,
  4,5,5,5,5,5,5,6,
  4,5,5,5,5,5,5,6,
  4,5,5,5,5,5,5,6,
  4,5,5,5,5,5,5,6,
  4,5,5,5,5,5,5,6,
  7,8,8,8,8,8,8,9
};

static const kvz_pixel pic_data[64] = {
  1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1
};

static kvz_picture *g_pic = 0;
static kvz_picture *g_ref = 0;
static kvz_picture *g_big_pic = 0;
static kvz_picture *g_big_ref = 0;
static kvz_picture *g_64x64_zero = 0;
static kvz_picture *g_64x64_max = 0;

static struct sad_test_env_t {
  int width;
  int height;
  void * tested_func;
  const strategy_t * strategy;
  char msg[255];
} sad_test_env;

//////////////////////////////////////////////////////////////////////////
// SETUP, TEARDOWN AND HELPER FUNCTIONS
static void setup_tests()
{
  g_pic = kvz_image_alloc(KVZ_CSP_420, 8, 8);
  for (int i = 0; i < 64; ++i) {
    g_pic->y[i] = pic_data[i] + 48;
  }

  g_ref = kvz_image_alloc(KVZ_CSP_420, 8, 8);
  for (int i = 0; i < 64; ++i) {
    g_ref->y[i] = ref_data[i] + 48;
  }

  g_big_pic = kvz_image_alloc(KVZ_CSP_420, 64, 64);
  for (int i = 0; i < 64*64; ++i) {
    g_big_pic->y[i] = (i*i / 32 + i) % 255;
    //g_big_pic->y[i] = i % 255;
  }

  g_big_ref = kvz_image_alloc(KVZ_CSP_420, 64, 64);
  for (int i = 0; i < 64 * 64; ++i) {
    g_big_ref->y[i] = (i*i / 16 + i) % 255;
    //g_big_ref->y[i] = (i / 2) % 255;
  }

  g_64x64_zero = kvz_image_alloc(KVZ_CSP_420, 64, 64);
  memset(g_64x64_zero->y, 0, 64 * 64 * sizeof(kvz_pixel));
  
  g_64x64_max = kvz_image_alloc(KVZ_CSP_420, 64, 64);
  memset(g_64x64_max->y, PIXEL_MAX, 64 * 64 * sizeof(kvz_pixel));
}

static void tear_down_tests()
{
  kvz_image_free(g_pic);
  kvz_image_free(g_ref);
  kvz_image_free(g_big_pic);
  kvz_image_free(g_big_ref);
  kvz_image_free(g_64x64_zero);
  kvz_image_free(g_64x64_max);
}


//////////////////////////////////////////////////////////////////////////
// OVERLAPPING BOUNDARY TESTS
TEST test_topleft(void)
{
  ASSERT_EQ(
    1*(4*4) + (2+4)*(4*4) + 5*(4*4) - 64,
    TEST_SAD(-3, -3));
  PASS();
}

TEST test_top(void)
{
  ASSERT_EQ(
    (1+3)*4 + 2*(6*4) + (4+6)*4 + 5*(6*4) - 64,
    TEST_SAD(0, -3));
  PASS();
}

TEST test_topright(void)
{
  ASSERT_EQ(
    3*(4*4) + (2+6)*(4*4) + 5*(4*4) - 64,
    TEST_SAD(3, -3));
  PASS();
}

TEST test_left(void)
{
  ASSERT_EQ(
    (1+7)*4 + 4*(6*4) + (2+8)*4 + 5*(6*4) - 64,
    TEST_SAD(-3, 0));
  PASS();
}

TEST test_no_offset(void)
{
  ASSERT_EQ(
    (1+3+7+9) + (2+4+6+8)*6 + 5*(6*6) - 64,
    TEST_SAD(0, 0));
  PASS();
}

TEST test_right(void)
{
  ASSERT_EQ(
    (3+9)*4 + 6*(4*6) + (2+8)*4 + 5*(6*4) - 64,
    TEST_SAD(3, 0));
  PASS();
}

TEST test_bottomleft(void)
{
  ASSERT_EQ(
    7*(4*4) + (4+8)*(4*4) + 5*(4*4) - 64,
    TEST_SAD(-3, 3));
  PASS();
}

TEST test_bottom(void)
{
  ASSERT_EQ(
    (7+9)*4 + 8*(6*4) + (4+6)*4 + 5*(6*4) - 64,
    TEST_SAD(0, 3));
  PASS();
}

TEST test_bottomright(void)
{
  ASSERT_EQ(
    9*(4*4) + (6+8)*(4*4) + 5*(4*4) - 64,
    TEST_SAD(3, 3));
  PASS();
}

//////////////////////////////////////////////////////////////////////////
// OUT OF FRAME TESTS

#define DIST 10
TEST test_topleft_out(void)
{
  ASSERT_EQ(
    1*(8*8) - 64,
    TEST_SAD(-DIST, -DIST));
  PASS();
}

TEST test_top_out(void)
{
  ASSERT_EQ(
    (1+3)*8 + 2*(6*8) - 64,
    TEST_SAD(0, -DIST));
  PASS();
}

TEST test_topright_out(void)
{
  ASSERT_EQ(
    3*(8*8) - 64,
    TEST_SAD(DIST, -DIST));
  PASS();
}

TEST test_left_out(void)
{
  ASSERT_EQ(
    (1+7)*8 + 4*(6*8) - 64,
    TEST_SAD(-DIST, 0));
  PASS();
}

TEST test_right_out(void)
{
  ASSERT_EQ(
    (3+9)*8 + 6*(6*8) - 64,
    TEST_SAD(DIST, 0));
  PASS();
}

TEST test_bottomleft_out(void)
{
  ASSERT_EQ(
    7*(8*8) - 64,
    TEST_SAD(-DIST, DIST));
  PASS();
}

TEST test_bottom_out(void)
{
  ASSERT_EQ(
    (7+9)*8 + 8*(6*8) - 64,
    TEST_SAD(0, DIST));
  PASS();
}

TEST test_bottomright_out(void)
{
  ASSERT_EQ(
    9*(8*8) - 64,
    TEST_SAD(DIST, DIST));
  PASS();
}

static unsigned simple_sad(const kvz_pixel* buf1, const kvz_pixel* buf2, unsigned stride,
                           unsigned width, unsigned height)
{
  unsigned sum = 0;
  for (unsigned y = 0; y < height; ++y) {
    for (unsigned x = 0; x < width; ++x) {
      sum += abs((int)buf1[y * stride + x] - (int)buf2[y * stride + x]);
    }
  }
  return sum;
}

TEST test_reg_sad(void)
{
  unsigned width = sad_test_env.width;
  unsigned height = sad_test_env.height;
  unsigned stride = 64;

  unsigned correct_result = simple_sad(g_big_pic->y, g_big_ref->y, stride, width, height);
  
  unsigned(*tested_func)(const kvz_pixel *, const kvz_pixel *, int, int, unsigned, unsigned) = sad_test_env.tested_func;
  unsigned result = tested_func(g_big_pic->y, g_big_ref->y, width, height, stride, stride);
  
  sprintf(sad_test_env.msg, "%s(%ux%u):%s",
          sad_test_env.strategy->type,
          width,
          height,
          sad_test_env.strategy->strategy_name);

  if (result != correct_result) {
    FAILm(sad_test_env.msg);
  }

  PASSm(sad_test_env.msg);
}


TEST test_reg_sad_overflow(void)
{
  unsigned width = sad_test_env.width;
  unsigned height = sad_test_env.height;
  unsigned stride = 64;

  unsigned correct_result = simple_sad(g_64x64_zero->y, g_64x64_max->y, stride, width, height);

  unsigned(*tested_func)(const kvz_pixel *, const kvz_pixel *, int, int, unsigned, unsigned) = sad_test_env.tested_func;
  unsigned result = tested_func(g_64x64_zero->y, g_64x64_max->y, width, height, stride, stride);

  sprintf(sad_test_env.msg, "overflow %s(%ux%u):%s",
    sad_test_env.strategy->type,
    width,
    height,
    sad_test_env.strategy->strategy_name);

  if (result != correct_result) {
    FAILm(sad_test_env.msg);
  }

  PASSm(sad_test_env.msg);
}


//////////////////////////////////////////////////////////////////////////
// TEST FIXTURES
SUITE(sad_tests)
{
  //SET_SETUP(sad_setup);
  //SET_TEARDOWN(sad_teardown);

  setup_tests();

  for (unsigned i = 0; i < strategies.count; ++i) {
    if (strcmp(strategies.strategies[i].type, "reg_sad") != 0) {
      continue;
    }

    // Change the global reg_sad function pointer.
    kvz_reg_sad = strategies.strategies[i].fptr;

    // Tests for movement vectors that overlap frame.
    RUN_TEST(test_topleft);
    RUN_TEST(test_top);
    RUN_TEST(test_topright);

    RUN_TEST(test_left);
    RUN_TEST(test_no_offset);
    RUN_TEST(test_right);

    RUN_TEST(test_bottomleft);
    RUN_TEST(test_bottom);
    RUN_TEST(test_bottomright);

    // Tests for movement vectors that are outside the frame.
    RUN_TEST(test_topleft_out);
    RUN_TEST(test_top_out);
    RUN_TEST(test_topright_out);

    RUN_TEST(test_left_out);
    RUN_TEST(test_right_out);

    RUN_TEST(test_bottomleft_out);
    RUN_TEST(test_bottom_out);
    RUN_TEST(test_bottomright_out);

    struct dimension {
      int width;
      int height;
    };
    static const struct dimension tested_dims[] = {
      // Square motion partitions
      {64, 64}, {32, 32}, {16, 16}, {8, 8},
      // Symmetric motion partitions
      {64, 32}, {32, 64}, {32, 16}, {16, 32}, {16, 8}, {8, 16}, {8, 4}, {4, 8},
      // Asymmetric motion partitions
      {48, 16}, {16, 48}, {24, 16}, {16, 24}, {12, 4}, {4, 12}
    };

    sad_test_env.tested_func = strategies.strategies[i].fptr;
    sad_test_env.strategy = &strategies.strategies[i];
    int num_dim_tests = sizeof(tested_dims) / sizeof(tested_dims[0]);
    for (volatile int dim_test = 0; dim_test < num_dim_tests; ++dim_test) {
      sad_test_env.width = tested_dims[dim_test].width;
      sad_test_env.height = tested_dims[dim_test].height;
      RUN_TEST(test_reg_sad);
      RUN_TEST(test_reg_sad_overflow);
    }
  }
  
  tear_down_tests();
}
