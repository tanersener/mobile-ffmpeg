/*
 *  Copyright (c) 2018 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

/*
 * Fuzzer for libvpx decoders
 * ==========================
 * Requirements
 * --------------
 * Requires Clang 6.0 or above as -fsanitize=fuzzer is used as a linker
 * option.

 * Steps to build
 * --------------
 * Clone libvpx repository
   $git clone https://chromium.googlesource.com/webm/libvpx

 * Create a directory in parallel to libvpx and change directory
   $mkdir vpx_dec_fuzzer
   $cd vpx_dec_fuzzer/

 * Enable sanitizers (Supported: address integer memory thread undefined)
   $source ../libvpx/tools/set_analyzer_env.sh address

 * Configure libvpx.
 * Note --size-limit and VPX_MAX_ALLOCABLE_MEMORY are defined to avoid
 * Out of memory errors when running generated fuzzer binary
   $../libvpx/configure --disable-unit-tests --size-limit=12288x12288 \
   --extra-cflags="-DVPX_MAX_ALLOCABLE_MEMORY=1073741824" \
   --disable-webm-io --enable-debug

 * Build libvpx
   $make -j32

 * Build vp9 fuzzer
   $ $CXX $CXXFLAGS -std=c++11 -DDECODER=vp9 \
   -fsanitize=fuzzer -I../libvpx -I. -Wl,--start-group \
   ../libvpx/examples/vpx_dec_fuzzer.cc -o ./vpx_dec_fuzzer_vp9 \
   ./libvpx.a ./tools_common.c.o -Wl,--end-group

 * DECODER should be defined as vp9 or vp8 to enable vp9/vp8
 *
 * create a corpus directory and copy some ivf files there.
 * Based on which codec (vp8/vp9) is being tested, it is recommended to
 * have corresponding ivf files in corpus directory
 * Empty corpus directoy also is acceptable, though not recommended
   $mkdir CORPUS && cp some-files CORPUS

 * Run fuzzing:
   $./vpx_dec_fuzzer_vp9 CORPUS

 * References:
 * http://llvm.org/docs/LibFuzzer.html
 * https://github.com/google/oss-fuzz
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory>

#include "./tools_common.h"
#include "vpx/vp8dx.h"
#include "vpx/vpx_decoder.h"
#include "vpx_ports/mem_ops.h"

#define VPX_TOSTRING(str) #str
#define VPX_STRINGIFY(str) VPX_TOSTRING(str)

static void CloseFile(FILE *file) { fclose(file); }

/* ReadFrame is derived from ivf_read_frame in ivfdec.c
 * This function doesn't call warn(), but instead ignores those errors.
 * This is done to minimize the prints on console when running fuzzer
 * Also if fread fails to read frame_size number of bytes, instead of
 * returning an error, this returns with partial frames.
 * This is done to ensure that partial frames are sent to decoder.
 */
static int ReadFrame(FILE *infile, uint8_t **buffer, size_t *bytes_read,
                     size_t *buffer_size) {
  char raw_header[IVF_FRAME_HDR_SZ] = { 0 };
  size_t frame_size = 0;

  if (fread(raw_header, IVF_FRAME_HDR_SZ, 1, infile) == 1) {
    frame_size = mem_get_le32(raw_header);

    if (frame_size > 256 * 1024 * 1024) {
      frame_size = 0;
    }

    if (frame_size > *buffer_size) {
      uint8_t *new_buffer = (uint8_t *)realloc(*buffer, 2 * frame_size);

      if (new_buffer) {
        *buffer = new_buffer;
        *buffer_size = 2 * frame_size;
      } else {
        frame_size = 0;
      }
    }
  }

  if (!feof(infile)) {
    *bytes_read = fread(*buffer, 1, frame_size, infile);
    return 0;
  }

  return 1;
}

extern "C" void usage_exit(void) { exit(EXIT_FAILURE); }

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  std::unique_ptr<FILE, decltype(&CloseFile)> file(
      fmemopen((void *)data, size, "rb"), &CloseFile);
  if (file == nullptr) {
    return 0;
  }
  // Ensure input contains at least one file header and one frame header
  if (size < IVF_FILE_HDR_SZ + IVF_FRAME_HDR_SZ) {
    return 0;
  }
  char header[IVF_FILE_HDR_SZ];
  if (fread(header, 1, IVF_FILE_HDR_SZ, file.get()) != IVF_FILE_HDR_SZ) {
    return 0;
  }
  const VpxInterface *decoder = get_vpx_decoder_by_name(VPX_STRINGIFY(DECODER));
  if (decoder == nullptr) {
    return 0;
  }

  vpx_codec_ctx_t codec;
  // Set thread count in the range [1, 64].
  const unsigned int threads = (data[IVF_FILE_HDR_SZ] & 0x3f) + 1;
  vpx_codec_dec_cfg_t cfg = { threads, 0, 0 };
  if (vpx_codec_dec_init(&codec, decoder->codec_interface(), &cfg, 0)) {
    return 0;
  }

  uint8_t *buffer = nullptr;
  size_t buffer_size = 0;
  size_t frame_size = 0;

  while (!ReadFrame(file.get(), &buffer, &frame_size, &buffer_size)) {
    const vpx_codec_err_t err =
        vpx_codec_decode(&codec, buffer, frame_size, nullptr, 0);
    static_cast<void>(err);
    vpx_codec_iter_t iter = nullptr;
    vpx_image_t *img = nullptr;
    while ((img = vpx_codec_get_frame(&codec, &iter)) != nullptr) {
    }
  }
  vpx_codec_destroy(&codec);
  free(buffer);
  return 0;
}
