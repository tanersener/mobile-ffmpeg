/*
 * Copyright (c) 2017, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

// Lightfield Decoder
// ==================
//
// This is an example of a simple lightfield decoder. It builds upon the
// simple_decoder.c example.  It takes an input file containing the compressed
// data (in ivf format), treating it as a lightfield instead of a video; and a
// text file with a list of tiles to decode.
// After running the lightfield encoder, run lightfield decoder to decode a
// batch of tiles:
// examples/lightfield_decoder vase10x10.ivf vase_reference.yuv 4 tile_list.txt
//
// The tile_list.txt is expected to be of the form:
// Frame <frame_index0>
// <image_index0> <anchor_index0> <tile_col0> <tile_row0>
// <image_index1> <anchor_index1> <tile_col1> <tile_row1>
// ...
// Frame <frame_index1)
// ...
//
// The "Frame" markers indicate a new render frame and thus a new tile list
// will be started and the old one flushed.  The image_indexN, anchor_indexN,
// tile_colN, and tile_rowN identify an individual tile to be decoded and
// to use anchor_indexN anchor image for MCP.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aom/aom_decoder.h"
#include "aom/aomdx.h"
#include "aom_scale/yv12config.h"
#include "av1/common/enums.h"
#include "common/tools_common.h"
#include "common/video_reader.h"

static const char *exec_name;

void usage_exit(void) {
  fprintf(stderr, "Usage: %s <infile> <outfile> <num_references> <tile_list>\n",
          exec_name);
  exit(EXIT_FAILURE);
}

void decode_tile(aom_codec_ctx_t *codec, const unsigned char *frame,
                 size_t frame_size, int tr, int tc, int ref_idx,
                 aom_image_t *reference_images, FILE *outfile) {
  aom_codec_control_(codec, AV1_SET_TILE_MODE, 1);
  aom_codec_control_(codec, AV1D_EXT_TILE_DEBUG, 1);
  aom_codec_control_(codec, AV1_SET_DECODE_TILE_ROW, tr);
  aom_codec_control_(codec, AV1_SET_DECODE_TILE_COL, tc);

  av1_ref_frame_t ref;
  ref.idx = 0;
  ref.use_external_ref = 1;
  ref.img = reference_images[ref_idx];
  if (aom_codec_control(codec, AV1_SET_REFERENCE, &ref)) {
    die_codec(codec, "Failed to set reference frame.");
  }

  aom_codec_err_t aom_status = aom_codec_decode(codec, frame, frame_size, NULL);
  if (aom_status) die_codec(codec, "Failed to decode tile.");

  aom_codec_iter_t iter = NULL;
  aom_image_t *img = aom_codec_get_frame(codec, &iter);
  aom_img_write(img, outfile);
}

int main(int argc, char **argv) {
  FILE *outfile = NULL;
  aom_codec_ctx_t codec;
  AvxVideoReader *reader = NULL;
  const AvxInterface *decoder = NULL;
  const AvxVideoInfo *info = NULL;
  int num_references;
  aom_image_t reference_images[MAX_EXTERNAL_REFERENCES];
  size_t frame_size = 0;
  const unsigned char *frame = NULL;
  int i, j;
  const char *tile_list_file = NULL;
  exec_name = argv[0];

  if (argc != 5) die("Invalid number of arguments.");

  reader = aom_video_reader_open(argv[1]);
  if (!reader) die("Failed to open %s for reading.", argv[1]);

  if (!(outfile = fopen(argv[2], "wb")))
    die("Failed to open %s for writing.", argv[2]);

  num_references = (int)strtol(argv[3], NULL, 0);
  tile_list_file = argv[4];

  info = aom_video_reader_get_info(reader);

  decoder = get_aom_decoder_by_fourcc(info->codec_fourcc);
  if (!decoder) die("Unknown input codec.");
  printf("Using %s\n", aom_codec_iface_name(decoder->codec_interface()));

  if (aom_codec_dec_init(&codec, decoder->codec_interface(), NULL, 0))
    die_codec(&codec, "Failed to initialize decoder.");

  if (aom_codec_control(&codec, AV1D_SET_IS_ANNEXB, info->is_annexb)) {
    die("Failed to set annex b status");
  }

  // Decode anchor frames.
  aom_codec_control_(&codec, AV1_SET_TILE_MODE, 0);
  for (i = 0; i < num_references; ++i) {
    aom_video_reader_read_frame(reader);
    frame = aom_video_reader_get_frame(reader, &frame_size);
    if (aom_codec_decode(&codec, frame, frame_size, NULL))
      die_codec(&codec, "Failed to decode frame.");

    if (i == 0) {
      aom_img_fmt_t ref_fmt = 0;
      if (aom_codec_control(&codec, AV1D_GET_IMG_FORMAT, &ref_fmt))
        die_codec(&codec, "Failed to get the image format");

      int frame_res[2];
      if (aom_codec_control(&codec, AV1D_GET_FRAME_SIZE, frame_res))
        die_codec(&codec, "Failed to get the image frame size");

      // Allocate memory to store decoded references. Allocate memory with the
      // border so that it can be used as a reference.
      for (j = 0; j < num_references; j++) {
        unsigned int border = AOM_BORDER_IN_PIXELS;
        if (!aom_img_alloc_with_border(&reference_images[j], ref_fmt,
                                       frame_res[0], frame_res[1], 32, 8,
                                       border)) {
          die("Failed to allocate references.");
        }
      }
    }

    if (aom_codec_control(&codec, AV1_COPY_NEW_FRAME_IMAGE,
                          &reference_images[i]))
      die_codec(&codec, "Failed to copy decoded reference frame");

    aom_codec_iter_t iter = NULL;
    aom_image_t *img = NULL;
    while ((img = aom_codec_get_frame(&codec, &iter)) != NULL) {
      char name[1024];
      snprintf(name, sizeof(name), "ref_%d.yuv", i);
      printf("writing ref image to %s, %d, %d\n", name, img->d_w, img->d_h);
      FILE *ref_file = fopen(name, "wb");
      aom_img_write(img, ref_file);
      fclose(ref_file);
    }
  }

  FILE *infile = aom_video_reader_get_file(reader);
  // Record the offset of the first camera image.
  const FileOffset camera_frame_pos = ftello(infile);

  printf("Loading compressed frames into memory.\n");

  // Count the frames in the lightfield.
  int num_frames = 0;
  while (aom_video_reader_read_frame(reader)) {
    ++num_frames;
  }
  if (num_frames < 1) die("Input light field has no frames.");

  // Read all of the lightfield frames into memory.
  unsigned char **frames =
      (unsigned char **)malloc(num_frames * sizeof(unsigned char *));
  size_t *frame_sizes = (size_t *)malloc(num_frames * sizeof(size_t));
  // Seek to the first camera image.
  fseeko(infile, camera_frame_pos, SEEK_SET);
  for (int f = 0; f < num_frames; ++f) {
    aom_video_reader_read_frame(reader);
    frame = aom_video_reader_get_frame(reader, &frame_size);
    frames[f] = (unsigned char *)malloc(frame_size * sizeof(unsigned char));
    memcpy(frames[f], frame, frame_size);
    frame_sizes[f] = frame_size;
  }
  printf("Read %d frames.\n", num_frames);

  printf("Decoding tile list from file.\n");
  char line[1024];
  FILE *tile_list_fptr = fopen(tile_list_file, "r");
  while ((fgets(line, 1024, tile_list_fptr)) != NULL) {
    if (line[0] == 'F') {
      continue;
    }

    int image_idx;
    int ref_idx;
    int tc;
    int tr;

    sscanf(line, "%d %d %d %d", &image_idx, &ref_idx, &tc, &tr);
    if (image_idx >= num_frames) {
      die("Tile list image_idx out of bounds: %d >= %d.", image_idx,
          num_frames);
    }
    if (ref_idx >= num_references) {
      die("Tile list ref_idx out of bounds: %d >= %d.", ref_idx,
          num_references);
    }
    frame = frames[image_idx];
    frame_size = frame_sizes[image_idx];
    decode_tile(&codec, frame, frame_size, tr, tc, ref_idx, reference_images,
                outfile);
  }

  for (i = 0; i < num_references; i++) aom_img_free(&reference_images[i]);
  for (int f = 0; f < num_frames; ++f) {
    free(frames[f]);
  }
  free(frame_sizes);
  free(frames);
  if (aom_codec_destroy(&codec)) die_codec(&codec, "Failed to destroy codec");
  aom_video_reader_close(reader);
  fclose(outfile);

  return EXIT_SUCCESS;
}
