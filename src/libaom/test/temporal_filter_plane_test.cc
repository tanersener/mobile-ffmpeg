/*
 * Copyright (c) 2019, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include <cmath>
#include <cstdlib>
#include <string>

#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

#include "config/aom_config.h"
#include "config/aom_dsp_rtcd.h"
#include "config/av1_rtcd.h"

#include "aom_ports/mem.h"
#include "test/acm_random.h"
#include "test/clear_system_state.h"
#include "test/register_state_check.h"
#include "test/util.h"
#include "test/function_equivalence_test.h"

using libaom_test::ACMRandom;
using libaom_test::FunctionEquivalenceTest;
using ::testing::Combine;
using ::testing::Range;
using ::testing::Values;
using ::testing::ValuesIn;

#if !CONFIG_REALTIME_ONLY
namespace {

typedef void (*temporal_filter_plane_func)(
    uint8_t *frame1, unsigned int stride, uint8_t *frame2, unsigned int stride2,
    int block_width, int block_height, int strength, double sigma,
    int decay_control, const int *blk_fw, int use_32x32,
    unsigned int *accumulator, uint16_t *count);
typedef libaom_test::FuncParam<temporal_filter_plane_func>
    TestTemporal_FilterPlane;

typedef ::testing::tuple<TestTemporal_FilterPlane, int> TemporalFilter_Params;

class TemporalFilterTest
    : public ::testing::TestWithParam<TemporalFilter_Params> {
 public:
  virtual ~TemporalFilterTest() {}
  virtual void SetUp() {
    params_ = GET_PARAM(0);
    rnd_.Reset(ACMRandom::DeterministicSeed());
    src1_ = reinterpret_cast<uint8_t *>(aom_memalign(8, 256 * 256));
    src2_ = reinterpret_cast<uint8_t *>(aom_memalign(8, 256 * 256));

    ASSERT_TRUE(src1_ != NULL);
    ASSERT_TRUE(src2_ != NULL);
  }

  virtual void TearDown() {
    libaom_test::ClearSystemState();
    aom_free(src1_);
    aom_free(src2_);
  }
  void RunTest(int isRandom, int width, int height, int run_times);

  void GenRandomData(int width, int height, int stride, int stride2) {
    for (int ii = 0; ii < height; ii++) {
      for (int jj = 0; jj < width; jj++) {
        src1_[ii * stride + jj] = rnd_.Rand8();
        src2_[ii * stride2 + jj] = rnd_.Rand8();
      }
    }
  }

  void GenExtremeData(int width, int height, int stride, uint8_t *data,
                      int stride2, uint8_t *data2, uint8_t val) {
    for (int ii = 0; ii < height; ii++) {
      for (int jj = 0; jj < width; jj++) {
        data[ii * stride + jj] = val;
        data2[ii * stride2 + jj] = (255 - val);
      }
    }
  }

 protected:
  TestTemporal_FilterPlane params_;
  uint8_t *src1_;
  uint8_t *src2_;
  ACMRandom rnd_;
};

void TemporalFilterTest::RunTest(int isRandom, int width, int height,
                                 int run_times) {
  aom_usec_timer ref_timer, test_timer;
  for (int k = 0; k < 3; k++) {
    int stride = 5 << rnd_(6);  // Up to 256 stride
    int stride2 = 5 << rnd_(6);

    while (stride < width) {  // Make sure it's valid
      stride = 5 << rnd_(6);
      stride2 = 5 << rnd_(6);
    }
    if (isRandom) {
      GenRandomData(width, height, stride, stride2);
    } else {
      const int msb = 8;  // Up to 12 bit input
      const int limit = (1 << msb) - 1;
      if (k == 0) {
        GenExtremeData(width, height, stride, src1_, stride2, src2_, limit);
      } else {
        GenExtremeData(width, height, stride, src1_, stride2, src2_, 0);
      }
    }
    int use32X32 = 1;
    int strength = rnd_(16);
    double sigma = 2.1002103677063437;
    int decay_control = 5;
    int blk_fw = rnd_(16);
    DECLARE_ALIGNED(16, unsigned int, accumulator_ref[1024 * 3]);
    DECLARE_ALIGNED(16, uint16_t, count_ref[1024 * 3]);
    memset(accumulator_ref, 0, 1024 * 3 * sizeof(accumulator_ref[0]));
    memset(count_ref, 0, 1024 * 3 * sizeof(count_ref[0]));
    DECLARE_ALIGNED(16, unsigned int, accumulator_mod[1024 * 3]);
    DECLARE_ALIGNED(16, uint16_t, count_mod[1024 * 3]);
    memset(accumulator_mod, 0, 1024 * 3 * sizeof(accumulator_mod[0]));
    memset(count_mod, 0, 1024 * 3 * sizeof(count_mod[0]));

    params_.ref_func(src1_, stride, src2_, stride2, width, height, strength,
                     sigma, decay_control, &blk_fw, use32X32, accumulator_ref,
                     count_ref);
    params_.tst_func(src1_, stride, src2_, stride2, width, height, strength,
                     sigma, decay_control, &blk_fw, use32X32, accumulator_mod,
                     count_mod);

    if (run_times > 1) {
      aom_usec_timer_start(&ref_timer);
      for (int j = 0; j < run_times; j++) {
        params_.ref_func(src1_, stride, src2_, stride2, width, height, strength,
                         sigma, decay_control, &blk_fw, use32X32,
                         accumulator_ref, count_ref);
      }
      aom_usec_timer_mark(&ref_timer);
      const int elapsed_time_c =
          static_cast<int>(aom_usec_timer_elapsed(&ref_timer));

      aom_usec_timer_start(&test_timer);
      for (int j = 0; j < run_times; j++) {
        params_.tst_func(src1_, stride, src2_, stride2, width, height, strength,
                         sigma, decay_control, &blk_fw, use32X32,
                         accumulator_mod, count_mod);
      }
      aom_usec_timer_mark(&test_timer);
      const int elapsed_time_simd =
          static_cast<int>(aom_usec_timer_elapsed(&test_timer));

      printf(
          "c_time=%d \t simd_time=%d \t "
          "gain=%f\t width=%d\t height=%d \n",
          elapsed_time_c, elapsed_time_simd,
          (float)((float)elapsed_time_c / (float)elapsed_time_simd), width,
          height);

    } else {
      for (int i = 0, l = 0; i < height; i++) {
        for (int j = 0; j < width; j++, l++) {
          EXPECT_EQ(accumulator_ref[l], accumulator_mod[l])
              << "Error:" << k << " SSE Sum Test [" << width << "x" << height
              << "] C accumulator does not match optimized accumulator.";
          EXPECT_EQ(count_ref[l], count_mod[l])
              << "Error:" << k << " SSE Sum Test [" << width << "x" << height
              << "] C count does not match optimized count.";
        }
      }
    }
  }
}

TEST_P(TemporalFilterTest, OperationCheck) {
  for (int height = 16; height <= 32; height = height * 2) {
    RunTest(1, height, height, 1);  // GenRandomData
  }
}

TEST_P(TemporalFilterTest, ExtremeValues) {
  for (int height = 16; height <= 32; height = height * 2) {
    RunTest(0, height, height, 1);
  }
}

TEST_P(TemporalFilterTest, DISABLED_Speed) {
  for (int height = 16; height <= 32; height = height * 2) {
    RunTest(1, height, height, 100000);
  }
}

#if HAVE_AVX2
TestTemporal_FilterPlane Temporal_filter_test_avx2[] = {
  TestTemporal_FilterPlane(&av1_temporal_filter_plane_c,
                           &av1_temporal_filter_plane_avx2)
};
INSTANTIATE_TEST_CASE_P(AVX2, TemporalFilterTest,
                        Combine(ValuesIn(Temporal_filter_test_avx2),
                                Range(64, 65, 4)));
#endif  // HAVE_AVX2

#if HAVE_SSE2
TestTemporal_FilterPlane Temporal_filter_test_sse2[] = {
  TestTemporal_FilterPlane(&av1_temporal_filter_plane_c,
                           &av1_temporal_filter_plane_sse2)
};
INSTANTIATE_TEST_CASE_P(SSE2, TemporalFilterTest,
                        Combine(ValuesIn(Temporal_filter_test_sse2),
                                Range(64, 65, 4)));
#endif  // HAVE_SSE2

}  // namespace
#endif
