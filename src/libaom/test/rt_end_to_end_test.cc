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

#include <memory>

#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

#include "test/codec_factory.h"
#include "test/encode_test_driver.h"
#include "test/util.h"
#include "test/y4m_video_source.h"
#include "test/yuv_video_source.h"

namespace {

const unsigned int kFrames = 10;
const int kBitrate = 500;

// List of psnr thresholds for speed settings 0-8
const double kPsnrThreshold[9] = { 37.0, 36.9, 36.8, 36.7, 36.5,
                                   36.3, 35.8, 35.2, 34.8 };

typedef struct {
  const char *filename;
  unsigned int input_bit_depth;
  aom_img_fmt fmt;
  aom_bit_depth_t bit_depth;
  unsigned int profile;
} TestVideoParam;

std::ostream &operator<<(std::ostream &os, const TestVideoParam &test_arg) {
  return os << "TestVideoParam { filename:" << test_arg.filename
            << " input_bit_depth:" << test_arg.input_bit_depth
            << " fmt:" << test_arg.fmt << " bit_depth:" << test_arg.bit_depth
            << " profile:" << test_arg.profile << "}";
}

// TODO(kyslov): Add more test vectors
const TestVideoParam kTestVectors[] = {
  { "park_joy_90p_8_420.y4m", 8, AOM_IMG_FMT_I420, AOM_BITS_8, 0 },
  { "paris_352_288_30.y4m", 8, AOM_IMG_FMT_I420, AOM_BITS_8, 0 },
};

class RTEndToEndTest
    : public ::libaom_test::CodecTestWith4Params<TestVideoParam, int,
                                                 unsigned int, int>,
      public ::libaom_test::EncoderTest {
 protected:
  RTEndToEndTest()
      : EncoderTest(GET_PARAM(0)), test_video_param_(GET_PARAM(1)),
        cpu_used_(GET_PARAM(2)), psnr_(0.0), nframes_(0),
        aq_mode_(GET_PARAM(3)), threads_(GET_PARAM(4)) {}

  virtual ~RTEndToEndTest() {}

  virtual void SetUp() {
    InitializeConfig();
    SetMode(::libaom_test::kRealTime);

    cfg_.g_usage =
        AOM_USAGE_REALTIME;  // TODO(kyslov): Move it to encode_test_driver.cc
    cfg_.rc_end_usage = AOM_CBR;
    cfg_.g_threads = threads_;
    cfg_.rc_buf_sz = 1000;
    cfg_.rc_buf_initial_sz = 500;
    cfg_.rc_buf_optimal_sz = 600;
  }

  virtual void BeginPassHook(unsigned int) {
    psnr_ = 0.0;
    nframes_ = 0;
  }

  virtual void PSNRPktHook(const aom_codec_cx_pkt_t *pkt) {
    psnr_ += pkt->data.psnr.psnr[0];
    nframes_++;
  }

  virtual void PreEncodeFrameHook(::libaom_test::VideoSource *video,
                                  ::libaom_test::Encoder *encoder) {
    if (video->frame() == 0) {
      encoder->Control(AV1E_SET_FRAME_PARALLEL_DECODING, 1);
      encoder->Control(AV1E_SET_TILE_COLUMNS, threads_);
      encoder->Control(AOME_SET_CPUUSED, cpu_used_);
      encoder->Control(AV1E_SET_TUNE_CONTENT, AOM_CONTENT_DEFAULT);
      encoder->Control(AV1E_SET_AQ_MODE, aq_mode_);
      encoder->Control(AV1E_SET_ROW_MT, 1);
    }
  }

  double GetAveragePsnr() const {
    if (nframes_) return psnr_ / nframes_;
    return 0.0;
  }

  double GetPsnrThreshold() { return kPsnrThreshold[cpu_used_]; }

  void DoTest() {
    cfg_.rc_target_bitrate = kBitrate;
    cfg_.g_error_resilient = 0;
    cfg_.g_profile = test_video_param_.profile;
    cfg_.g_input_bit_depth = test_video_param_.input_bit_depth;
    cfg_.g_bit_depth = test_video_param_.bit_depth;
    init_flags_ = AOM_CODEC_USE_PSNR;
    if (cfg_.g_bit_depth > 8) init_flags_ |= AOM_CODEC_USE_HIGHBITDEPTH;

    std::unique_ptr<libaom_test::VideoSource> video;
    video.reset(new libaom_test::Y4mVideoSource(test_video_param_.filename, 0,
                                                kFrames));
    ASSERT_TRUE(video.get() != NULL);

    ASSERT_NO_FATAL_FAILURE(RunLoop(video.get()));
    const double psnr = GetAveragePsnr();
    EXPECT_GT(psnr, GetPsnrThreshold())
        << "cpu used = " << cpu_used_ << " aq mode = " << aq_mode_;
  }

  TestVideoParam test_video_param_;
  int cpu_used_;

 private:
  double psnr_;
  unsigned int nframes_;
  unsigned int aq_mode_;
  unsigned int threads_;
};

// *Threaded* and *Large* tests are used to simplify test filtering.
class RTEndToEndTestLarge : public RTEndToEndTest {};

class RTEndToEndTestThreadedLarge : public RTEndToEndTest {};

class RTEndToEndTestThreaded : public RTEndToEndTest {};

TEST_P(RTEndToEndTestLarge, EndtoEndPSNRTest) { DoTest(); }

TEST_P(RTEndToEndTestThreadedLarge, EndtoEndPSNRTest) { DoTest(); }

TEST_P(RTEndToEndTest, EndtoEndPSNRTest) { DoTest(); }

TEST_P(RTEndToEndTestThreaded, EndtoEndPSNRTest) { DoTest(); }

AV1_INSTANTIATE_TEST_CASE(RTEndToEndTestLarge,
                          ::testing::ValuesIn(kTestVectors),
                          ::testing::Range(0, 7),
                          ::testing::Values<unsigned int>(0),
                          ::testing::Values(1));

AV1_INSTANTIATE_TEST_CASE(RTEndToEndTestThreadedLarge,
                          ::testing::ValuesIn(kTestVectors),
                          ::testing::Range(0, 7),
                          ::testing::Values<unsigned int>(0),
                          ::testing::Values(2, 5));

AV1_INSTANTIATE_TEST_CASE(RTEndToEndTest, ::testing::Values(kTestVectors[0]),
                          ::testing::Range(7, 9),
                          ::testing::Range<unsigned int>(0, 4),
                          ::testing::Values(1));

AV1_INSTANTIATE_TEST_CASE(RTEndToEndTestThreaded,
                          ::testing::Values(kTestVectors[1]),
                          ::testing::Range(7, 9),
                          ::testing::Range<unsigned int>(0, 4),
                          ::testing::Range(2, 5));
}  // namespace
