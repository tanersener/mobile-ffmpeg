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
#include "test/util.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace {

using ::testing::get;
using ::testing::tuple;

/** Get the (i, j) value from the input; if i or j is outside of the width
 * or height, the nearest pixel value is returned.
 */

static int get_nearest_pix(const uint8_t *buf, int w, int h, int i, int j,
                           int bd) {
  int offset = AOMMAX(AOMMIN(i, w - 1), 0) + w * AOMMAX(AOMMIN(j, h - 1), 0);
  if (bd <= 8) {
    return buf[offset];
  } else {
    return *CONVERT_TO_SHORTPTR(buf + offset);
  }
}

static int get_pix(uint8_t *buf, int i, int bd) {
  if (bd <= 8) {
    return buf[i];
  } else {
    return *CONVERT_TO_SHORTPTR(buf + i);
  }
}

static void set_pix(uint8_t *buf, int i, int v, int bd) {
  if (bd <= 8) {
    buf[i] = v;
  } else {
    *CONVERT_TO_SHORTPTR(buf + i) = v;
  }
}

/** Given the image data, creates a new image with padded values, so an
 * 8-tap filter can be convolved. The padded value is the same as the closest
 * value in the image. Returns a pointer to the start of the image in the
 * padded data. Must be freed with free_pad_8tap.
 */
static uint8_t *pad_8tap_convolve(const uint8_t *data, int w, int h, int bd) {
  // SIMD optimizations require the width to be a multiple of 8 and the height
  // to be multiples of 4.
  assert(w % 8 == 0);
  assert(h % 4 == 0);
  // For an 8-tap filter, we need to pad with 3 lines on top and on the left,
  // and 4 lines on the right and bottom, for 7 extra lines.
  const int pad_w = w + 7;
  const int pad_h = h + 7;

  uint8_t *dst;
  if (bd <= 8) {
    dst = (uint8_t *)aom_memalign(32, sizeof(uint8_t) * pad_w * pad_h);
  } else {
    dst =
        CONVERT_TO_BYTEPTR(aom_memalign(32, sizeof(uint16_t) * pad_w * pad_h));
  }

  for (int j = 0; j < pad_h; ++j) {
    for (int i = 0; i < pad_w; ++i) {
      const int v = get_nearest_pix(data, w, h, i - 3, j - 3, bd);
      if (bd <= 8) {
        dst[i + j * pad_w] = v;
      } else {
        *CONVERT_TO_SHORTPTR(dst + i + j * pad_w) = v;
      }
    }
  }
  return dst + (w + 7) * 3 + 3;
}

static int stride_8tap(int width) { return width + 7; }

static void free_pad_8tap(uint8_t *padded, int width, int bd) {
  if (bd <= 8) {
    aom_free(padded - (width + 7) * 3 - 3);
  } else {
    aom_free(CONVERT_TO_SHORTPTR(padded - (width + 7) * 3 - 3));
  }
}

static uint8_t *malloc_bd(int num_entries, int bd) {
  const int bytes_per_entry = bd <= 8 ? sizeof(uint8_t) : sizeof(uint16_t);

  uint8_t *buf = (uint8_t *)aom_memalign(32, bytes_per_entry * num_entries);
  if (bd <= 8) {
    return buf;
  } else {
    return CONVERT_TO_BYTEPTR(buf);
  }
}

static void free_bd(uint8_t *p, int bd) {
  if (bd <= 8) {
    aom_free(p);
  } else {
    aom_free(CONVERT_TO_SHORTPTR(p));
  }
}

class EdgeDetectBrightnessTest :
    // Parameters are (brightness, width, height, bit depth).
    public ::testing::TestWithParam<tuple<int, int, int, int> > {
 protected:
  void SetUp() override {
    // Allocate a (width by height) array of luma values in orig_.
    // padded_ will be filled by the pad() call, which adds a border around
    // the orig_. The output_ array has enough space for the computation.
    const int width = GET_PARAM(1);
    const int height = GET_PARAM(2);
    const int bd = GET_PARAM(3);
    orig_ = malloc_bd(width * height, bd);
    padded_ = nullptr;
    output_ = malloc_bd(width * height, bd);
  }

  void TearDown() override {
    const int bd = GET_PARAM(3);
    if (orig_ != nullptr) {
      free_bd(orig_, bd);
    }
    if (padded_ != nullptr) {
      const int width = GET_PARAM(1);
      free_pad_8tap(padded_, width, bd);
    }
    free_bd(output_, bd);
  }

  void pad() {
    const int width = GET_PARAM(1);
    const int height = GET_PARAM(2);
    const int bd = GET_PARAM(3);
    padded_ = pad_8tap_convolve(orig_, width, height, bd);
    // Get rid of the original buffer, it should not be used further.
    free_bd(orig_, bd);
    orig_ = nullptr;
  }

  uint8_t *orig_;
  uint8_t *padded_;
  uint8_t *output_;
};

TEST_P(EdgeDetectBrightnessTest, BlurUniformBrightness) {
  // For varying levels of brightness, the algorithm should
  // produce the same output.
  const int brightness = GET_PARAM(0);
  const int width = GET_PARAM(1);
  const int height = GET_PARAM(2);
  const int bd = GET_PARAM(3);
  // Skip the tests where brightness exceeds the bit-depth; we run into this
  // issue because of gtest's limitation on valid combinations of test
  // parameters.
  if (brightness >= (1 << bd)) {
    return;
  }
  for (int i = 0; i < width * height; ++i) {
    set_pix(orig_, i, brightness, bd);
  }
  pad();
  gaussian_blur(padded_, stride_8tap(width), width, height, output_, bd);
  for (int i = 0; i < width * height; ++i) {
    ASSERT_EQ(brightness, get_pix(output_, i, bd));
  }
}

// No edges on a uniformly bright image.
TEST_P(EdgeDetectBrightnessTest, DetectUniformBrightness) {
  const int brightness = GET_PARAM(0);
  const int width = GET_PARAM(1);
  const int height = GET_PARAM(2);
  const int bd = GET_PARAM(3);
  // Skip the tests where brightness exceeds the bit-depth; we run into this
  // issue because of gtest's limitation on valid combinations of test
  // parameters.
  if (brightness >= (1 << bd)) {
    return;
  }
  for (int i = 0; i < width * height; ++i) {
    set_pix(orig_, i, brightness, bd);
  }
  pad();
  ASSERT_EQ(0, av1_edge_exists(padded_, stride_8tap(width), width, height, bd));
}

INSTANTIATE_TEST_CASE_P(ImageBrightnessTests, EdgeDetectBrightnessTest,
                        ::testing::Combine(
                            // Brightness
                            ::testing::Values(0, 1, 2, 127, 128, 129, 254, 255,
                                              256, 511, 512, 1023, 1024, 2048,
                                              4095),
                            // Width
                            ::testing::Values(8, 16, 32),
                            // Height
                            ::testing::Values(4, 8, 12, 32),
                            // Bit depth
                            ::testing::Values(8, 10, 12)));

class EdgeDetectImageTest :
    // Parameters are (width, height, bit depth).
    public ::testing::TestWithParam<tuple<int, int, int> > {};

// Generate images with black on one side and white on the other.
TEST_P(EdgeDetectImageTest, BlackWhite) {
  const int width = GET_PARAM(0);
  const int height = GET_PARAM(1);
  const int bd = GET_PARAM(2);
  const int white = (1 << bd) - 1;
  uint8_t *orig = malloc_bd(width * height, bd);
  for (int j = 0; j < height; ++j) {
    for (int i = 0; i < width; ++i) {
      if (i < width / 2) {
        set_pix(orig, i + j * width, 0, bd);
      } else {
        set_pix(orig, i + j * width, white, bd);
      }
    }
  }
  uint8_t *padded = pad_8tap_convolve(orig, width, height, bd);
  free_bd(orig, bd);
  // Value should be between 556 and 560.
  ASSERT_LE(556,
            av1_edge_exists(padded, stride_8tap(width), width, height, bd));
  ASSERT_GE(560,
            av1_edge_exists(padded, stride_8tap(width), width, height, bd));

  free_pad_8tap(padded, width, bd);
}

// Hardcoded blur tests.
static const uint8_t luma[32] = { 241, 147, 7,   90,  184, 103, 28,  186,
                                  2,   248, 49,  242, 114, 146, 127, 22,
                                  121, 228, 167, 108, 158, 174, 41,  168,
                                  214, 99,  184, 109, 114, 247, 117, 119 };
static const uint8_t expected[] = { 161, 138, 119, 118, 123, 118, 113, 122,
                                    143, 140, 134, 133, 134, 126, 116, 114,
                                    147, 149, 145, 142, 143, 138, 126, 118,
                                    164, 156, 148, 144, 148, 148, 138, 126 };

TEST(EdgeDetectImageTest, HardcodedBlurTest) {
  const int w = 8;
  const int h = 4;
  int bd = 8;
  uint8_t *output = malloc_bd(w * h, bd);
  uint8_t *padded = pad_8tap_convolve(luma, w, h, bd);
  gaussian_blur(padded, stride_8tap(w), w, h, output, bd);
  for (int i = 0; i < w * h; ++i) {
    ASSERT_EQ(expected[i], get_pix(output, i, bd));
  }
  free_pad_8tap(padded, w, bd);
  free_bd(output, bd);

  // High bit-depth tests.
  for (bd = 10; bd <= 12; bd += 2) {
    uint16_t luma16[32];
    for (int i = 0; i < 32; ++i) {
      luma16[i] = luma[i];
    }
    uint8_t *output = malloc_bd(w * h, bd);
    uint8_t *padded = pad_8tap_convolve(CONVERT_TO_BYTEPTR(luma16), w, h, bd);
    gaussian_blur(padded, stride_8tap(w), w, h, output, bd);
    for (int i = 0; i < w * h; ++i) {
      ASSERT_EQ(expected[i], get_pix(output, i, bd));
    }
    free_pad_8tap(padded, w, bd);
    free_bd(output, bd);
  }
  // If we multiply the inputs by a constant factor, the output should not vary
  // more than 0.5 * factor.
  for (bd = 10; bd <= 12; bd += 2) {
    for (int c = 2; c < (1 << (bd - 8)); ++c) {
      uint16_t luma16[32];
      for (int i = 0; i < 32; ++i) {
        luma16[i] = luma[i] * c;
      }
      uint8_t *output = malloc_bd(w * h, bd);
      uint8_t *padded = pad_8tap_convolve(CONVERT_TO_BYTEPTR(luma16), w, h, bd);
      gaussian_blur(padded, stride_8tap(w), w, h, output, bd);
      for (int i = 0; i < w * h; ++i) {
        ASSERT_GE(c / 2, abs(expected[i] * c - get_pix(output, i, bd)));
      }
      free_pad_8tap(padded, w, bd);
      free_bd(output, bd);
    }
  }
}

TEST(EdgeDetectImageTest, HardcodedHighBdBlurTest) {
  // Randomly generated 8x4.
  const uint16_t luma[32] = { 241, 147, 7,   90,  184, 103, 28,  186,
                              2,   248, 49,  242, 114, 146, 127, 22,
                              121, 228, 167, 108, 158, 174, 41,  168,
                              214, 99,  184, 109, 114, 247, 117, 119 };
  uint16_t expected[] = { 161, 138, 119, 118, 123, 118, 113, 122, 143, 140, 134,
                          133, 134, 126, 116, 114, 147, 149, 145, 142, 143, 138,
                          126, 118, 164, 156, 148, 144, 148, 148, 138, 126 };
  const int w = 8;
  const int h = 4;
  for (int bd = 10; bd <= 12; bd += 2) {
    uint8_t *padded = pad_8tap_convolve(CONVERT_TO_BYTEPTR(luma), w, h, bd);
    uint8_t *output = malloc_bd(w * h, bd);
    gaussian_blur(padded, stride_8tap(w), w, h, output, bd);

    for (int i = 0; i < w * h; ++i) {
      ASSERT_EQ(expected[i], get_pix(output, i, bd));
    }
    free_pad_8tap(padded, w, bd);
    free_bd(output, bd);
  }
}

TEST(EdgeDetectImageTest, SobelTest) {
  // Randomly generated 3x3. Compute Sobel for middle value.
  const uint8_t buf[9] = { 241, 147, 7, 90, 184, 103, 28, 186, 2 };
  const int stride = 3;
  int bd = 8;
  sobel_xy result = sobel(buf, stride, 1, 1, bd);
  ASSERT_EQ(234, result.x);
  ASSERT_EQ(140, result.y);

  // Verify it works for high bit-depth values as well.
  const uint16_t buf16[9] = { 241, 147, 7, 90, 184, 2003, 1028, 186, 2 };
  for (bd = 10; bd <= 12; bd += 2) {
    result = sobel(CONVERT_TO_BYTEPTR(buf16), stride, 1, 1, bd);
    ASSERT_EQ(-2566, result.x);
    ASSERT_EQ(-860, result.y);
  }
}

INSTANTIATE_TEST_CASE_P(EdgeDetectImages, EdgeDetectImageTest,
                        ::testing::Combine(
                            // Width
                            ::testing::Values(8, 16, 32),
                            // Height
                            ::testing::Values(4, 8, 12, 32),
                            // Bit depth
                            ::testing::Values(8, 10, 12)));
}  // namespace
