// Copyright 2017 Google Inc. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the COPYING file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS. All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.
// -----------------------------------------------------------------------------
//
// Decodes an animated WebP file and dumps the decoded frames as PNG or TIFF.
//
// Author: Skal (pascal.massimino@gmail.com)

#include <stdio.h>
#include <string.h>  // for 'strcmp'.

#include "./anim_util.h"
#include "webp/decode.h"
#include "../imageio/image_enc.h"

#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#endif

static void Help(void) {
  printf("Usage: anim_dump [options] files...\n");
  printf("\nOptions:\n");
  printf("  -folder <string> .... dump folder (default: '.')\n");
  printf("  -prefix <string> .... prefix for dumped frames "
                                  "(default: 'dump_')\n");
  printf("  -tiff ............... save frames as TIFF\n");
  printf("  -pam ................ save frames as PAM\n");
  printf("  -h .................. this help\n");
  printf("  -version ............ print version number and exit\n");
}

int main(int argc, const char* argv[]) {
  int error = 0;
  const char* dump_folder = ".";
  const char* prefix = "dump_";
  const char* suffix = "png";
  WebPOutputFileFormat format = PNG;
  int c;

  if (argc < 2) {
    Help();
    return -1;
  }

  for (c = 1; !error && c < argc; ++c) {
    if (!strcmp(argv[c], "-folder")) {
      if (c + 1 == argc) {
        fprintf(stderr, "missing argument after option '%s'\n", argv[c]);
        error = 1;
        break;
      }
      dump_folder = argv[++c];
    } else if (!strcmp(argv[c], "-prefix")) {
      if (c + 1 == argc) {
        fprintf(stderr, "missing argument after option '%s'\n", argv[c]);
        error = 1;
        break;
      }
      prefix = argv[++c];
    } else if (!strcmp(argv[c], "-tiff")) {
      format = TIFF;
      suffix = "tiff";
    } else if (!strcmp(argv[c], "-pam")) {
      format = PAM;
      suffix = "pam";
    } else if (!strcmp(argv[c], "-h") || !strcmp(argv[c], "-help")) {
      Help();
      return 0;
    } else if (!strcmp(argv[c], "-version")) {
      int dec_version, demux_version;
      GetAnimatedImageVersions(&dec_version, &demux_version);
      printf("WebP Decoder version: %d.%d.%d\nWebP Demux version: %d.%d.%d\n",
             (dec_version >> 16) & 0xff, (dec_version >> 8) & 0xff,
             (dec_version >> 0) & 0xff,
             (demux_version >> 16) & 0xff, (demux_version >> 8) & 0xff,
             (demux_version >> 0) & 0xff);
      return 0;
    } else {
      uint32_t i;
      AnimatedImage image;
      const char* const file = argv[c];
      memset(&image, 0, sizeof(image));
      printf("Decoding file: %s as %s/%sxxxx.%s\n",
             file, dump_folder, prefix, suffix);
      if (!ReadAnimatedImage(file, &image, 0, NULL)) {
        fprintf(stderr, "Error decoding file: %s\n Aborting.\n", file);
        error = 1;
        break;
      }
      for (i = 0; !error && i < image.num_frames; ++i) {
        char out_file[1024];
        WebPDecBuffer buffer;
        WebPInitDecBuffer(&buffer);
        buffer.colorspace = MODE_RGBA;
        buffer.is_external_memory = 1;
        buffer.width = image.canvas_width;
        buffer.height = image.canvas_height;
        buffer.u.RGBA.rgba = image.frames[i].rgba;
        buffer.u.RGBA.stride = buffer.width * sizeof(uint32_t);
        buffer.u.RGBA.size = buffer.u.RGBA.stride * buffer.height;
        snprintf(out_file, sizeof(out_file), "%s/%s%.4d.%s",
                 dump_folder, prefix, i, suffix);
        if (!WebPSaveImage(&buffer, format, out_file)) {
          fprintf(stderr, "Error while saving image '%s'\n", out_file);
          error = 1;
        }
        WebPFreeDecBuffer(&buffer);
      }
      ClearAnimatedImage(&image);
    }
  }
  return error ? 1 : 0;
}
