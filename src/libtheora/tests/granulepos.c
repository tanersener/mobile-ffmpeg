/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2009                *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function: routines for validating encoder granulepos generation
  last mod: $Id: granulepos.c 16503 2009-08-22 18:14:02Z giles $

 ********************************************************************/

#include <stdlib.h>
#include <theora/theoraenc.h>
#include <math.h>

#include "tests.h"

static int ilog(unsigned int v){
  int ret=0;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}

static int
granulepos_test_encode (int frequency)
{
  th_info ti;
  th_enc_ctx *te;
  int result;
  int frame, tframe, keyframe, keydist;
  int shift;
  double rate, ttime;
  th_ycbcr_buffer yuv;
  unsigned char *framedata;
  ogg_packet op;
  long long int last_granule = -1;

/*  INFO ("+ Initializing th_info struct"); */
  th_info_init (&ti);

  ti.frame_width = 32;
  ti.frame_height = 32;
  ti.pic_width = ti.frame_width;
  ti.pic_height = ti.frame_height;
  ti.pic_x = 0;
  ti.pic_y = 0;
  ti.fps_numerator = 16;
  ti.fps_denominator = 1;
  ti.aspect_numerator = 1;
  ti.aspect_denominator = 1;
  ti.colorspace = TH_CS_UNSPECIFIED;
  ti.pixel_fmt = TH_PF_420;
  ti.quality = 16;
  ti.keyframe_granule_shift=ilog(frequency);

/*  INFO ("+ Allocating encoder context"); */
  te = th_encode_alloc(&ti);
  if (te == NULL) {
    INFO ("+ Clearing th_info");
    th_info_clear(&ti);
    FAIL ("negative return code initializing encoder");
  }

/*  INFO ("+ Setting up dummy 4:2:0 frame data"); */
  framedata = calloc(ti.frame_height, ti.frame_width);
  yuv[0].width = ti.frame_width;
  yuv[0].height = ti.frame_height;
  yuv[0].stride = ti.frame_width;
  yuv[0].data = framedata;
  yuv[1].width = ti.frame_width / 2;
  yuv[1].height = ti.frame_width / 2;
  yuv[1].stride = ti.frame_width;
  yuv[1].data = framedata;
  yuv[2].width = ti.frame_width / 2;
  yuv[2].height = ti.frame_width / 2;
  yuv[2].stride = ti.frame_width;
  yuv[2].data = framedata;

  INFO ("+ Checking granulepos generation");
  shift = ti.keyframe_granule_shift;
  rate = (double)ti.fps_denominator/ti.fps_numerator;
  for (frame = 0; frame < frequency * 2 + 1; frame++) {
    result = th_encode_ycbcr_in (te, yuv);
    if (result < 0) {
      printf("th_encode_ycbcr_in() returned %d\n", result);
      FAIL ("negative error code submitting frame for compression");
    }
    result = th_encode_packetout (te, frame >= frequency * 2, &op);
    if (result <= 0) {
      printf("th_encode_packetout() returned %d\n", result);
      FAIL("failed to retrieve compressed frame");
    }
    if ((long long int)op.granulepos < last_granule)
      FAIL ("encoder returned a decreasing granulepos value");
    last_granule = op.granulepos;
    keyframe = op.granulepos >> shift;
    keydist = op.granulepos - (keyframe << shift);
    tframe = th_granule_frame (te, op.granulepos);
    ttime = th_granule_time(te, op.granulepos);
#if DEBUG
    printf("++ frame %d granulepos %lld %d:%d %d %.3lfs\n",
	frame, (long long int)op.granulepos, keyframe, keydist,
	tframe, th_granule_time (te, op.granulepos));
#endif
    /* granulepos stores the frame count */
    if ((keyframe + keydist) != frame + 1)
      FAIL ("encoder granulepos does not map to the correct frame number");
    /* th_granule_frame() returns the frame index */
    if (tframe != frame)
      FAIL ("th_granule_frame() returned incorrect results");
    /* th_granule_time() returns the end time */
    if (fabs(rate*(frame+1) - ttime) > 1.0e-6)
      FAIL ("th_granule_time() returned incorrect results");
  }

  /* clean up */
/*  INFO ("+ Freeing dummy frame data"); */
  free(framedata);

/*  INFO ("+ Clearing th_info struct"); */
  th_info_clear(&ti);

/*  INFO ("+ Freeing encoder context"); */
  th_encode_free(te);

  return 0;
}

int main(int argc, char *argv[])
{

  granulepos_test_encode (1);
  granulepos_test_encode (2);
  granulepos_test_encode (3);
  granulepos_test_encode (4);
  granulepos_test_encode (8);
  granulepos_test_encode (64);

  exit (0);
}
