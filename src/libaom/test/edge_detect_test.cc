/*
 * Copyright (c) 2018, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include "aom_mem/aom_mem.h"
#include "av1/encoder/rdopt.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace {

using ::testing::get;
using ::testing::tuple;

class EdgeDetectBrightnessTest :
    // Parameters are (brightness, width, height).
    public ::testing::TestWithParam<tuple<int, int, int> > {};

/** Get the (x, y) value from the input; if i or j is outside of the width
 * or height, the nearest pixel value is returned.
 */
static uint8_t get_xy(const uint8_t *data, int w, int h, int i, int j) {
  return data[AOMMAX(AOMMIN(i, w - 1), 0) + w * AOMMAX(AOMMIN(j, h - 1), 0)];
}

/** Given the image data, creates a new image with padded values, so an
 * 8-tap filter can be convolved. The padded value is the same as the closest
 * value in the image. Returns a pointer to the start of the image in the
 * padded data. Must be freed with free_pad_8tap.
 */
uint8_t *pad_8tap_convolve(const uint8_t *data, int w, int h) {
  // SIMD optimizations require the width to be a multiple of 8 and the height
  // to be multiples of 4.
  assert(w % 8 == 0);
  assert(h % 4 == 0);
  // For an 8-tap filter, we need to pad with 3 lines on top and on the left,
  // and 4 lines on the right and bottom, for 7 extra lines.
  const int pad_w = w + 7;
  const int pad_h = h + 7;
  uint8_t *dst = (uint8_t *)aom_memalign(32, pad_w * pad_h);
  // Fill in the data from the original.
  for (int j = 0; j < pad_h; ++j) {
    for (int i = 0; i < pad_w; ++i) {
      dst[i + j * pad_w] = get_xy(data, w, h, i - 3, j - 3);
    }
  }
  return dst + (w + 7) * 3 + 3;
}

static void free_pad_8tap(uint8_t *padded, int width) {
  aom_free(padded - (width + 7) * 3 - 3);
}

static int stride_8tap(int width) { return width + 7; }

TEST_P(EdgeDetectBrightnessTest, BlurUniformBrightness) {
  // For varying levels of brightness, the algorithm should
  // produce the same output.
  const int brightness = get<0>(GetParam());
  const int width = get<1>(GetParam());
  const int height = get<2>(GetParam());
  uint8_t *orig = (uint8_t *)malloc(width * height);
  for (int i = 0; i < width * height; ++i) {
    orig[i] = brightness;
  }
  uint8_t *padded = pad_8tap_convolve(orig, width, height);
  free(orig);
  uint8_t *output = (uint8_t *)aom_memalign(32, width * height);
  gaussian_blur(padded, stride_8tap(width), width, height, output);
  for (int i = 0; i < width * height; ++i) {
    ASSERT_EQ(brightness, output[i]);
  }
  free_pad_8tap(padded, width);
  aom_free(output);
}

// No edges on a uniformly bright image.
TEST_P(EdgeDetectBrightnessTest, DetectUniformBrightness) {
  const int brightness = get<0>(GetParam());
  const int width = get<1>(GetParam());
  const int height = get<2>(GetParam());
  uint8_t *orig = (uint8_t *)malloc(width * height);
  for (int i = 0; i < width * height; ++i) {
    orig[i] = brightness;
  }
  uint8_t *padded = pad_8tap_convolve(orig, width, height);
  free(orig);
  ASSERT_EQ(0, av1_edge_exists(padded, stride_8tap(width), width, height));
  free_pad_8tap(padded, width);
}

INSTANTIATE_TEST_CASE_P(ImageBrightnessTests, EdgeDetectBrightnessTest,
                        ::testing::Combine(
                            // Brightness
                            ::testing::Values(0, 1, 2, 127, 128, 129, 254, 255),
                            // Width
                            ::testing::Values(8, 16, 32),
                            // Height
                            ::testing::Values(4, 8, 12, 32)));

class EdgeDetectImageTest :
    // Parameters are (width, height).
    public ::testing::TestWithParam<tuple<int, int> > {};

// Generate images with black on one side and white on the other.
TEST_P(EdgeDetectImageTest, BlackWhite) {
  const int width = get<0>(GetParam());
  const int height = get<1>(GetParam());
  uint8_t *orig = (uint8_t *)malloc(width * height);
  for (int j = 0; j < height; ++j) {
    for (int i = 0; i < width; ++i) {
      if (i < width / 2) {
        orig[i + j * width] = 0;
      } else {
        orig[i + j * width] = 255;
      }
    }
  }
  uint8_t *padded = pad_8tap_convolve(orig, width, height);
  free(orig);
  ASSERT_LE(556, av1_edge_exists(padded, stride_8tap(width), width, height));
  free_pad_8tap(padded, width);
}

TEST(EdgeDetectImageTest, HardcodedBlurTest) {
  // Randomly generated 8x4.
  const uint8_t luma[32] = { 241, 147, 7,   90,  184, 103, 28,  186,
                             2,   248, 49,  242, 114, 146, 127, 22,
                             121, 228, 167, 108, 158, 174, 41,  168,
                             214, 99,  184, 109, 114, 247, 117, 119 };
  uint8_t expected[] = { 161, 138, 119, 118, 123, 118, 113, 122, 143, 140, 134,
                         133, 134, 126, 116, 114, 147, 149, 145, 142, 143, 138,
                         126, 118, 164, 156, 148, 144, 148, 148, 138, 126 };
  const int w = 8;
  const int h = 4;
  uint8_t *padded = pad_8tap_convolve(luma, w, h);
  uint8_t *output = (uint8_t *)aom_memalign(32, w * h);
  gaussian_blur(padded, stride_8tap(w), w, h, output);

  for (int i = 0; i < w * h; ++i) {
    ASSERT_EQ(expected[i], output[i]);
  }

  free_pad_8tap(padded, w);
  aom_free(output);
}

INSTANTIATE_TEST_CASE_P(EdgeDetectImages, EdgeDetectImageTest,
                        ::testing::Combine(
                            // Width
                            ::testing::Values(8, 16, 32),
                            // Height
                            ::testing::Values(4, 8, 12, 32)));

}  // namespace
