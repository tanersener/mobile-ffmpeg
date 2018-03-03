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
  last mod: $Id: granulepos_theora.c 16503 2009-08-22 18:14:02Z giles $

 ********************************************************************/

#include <stdlib.h>
#include <theora/theora.h>
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
granulepos_test_encode (int frequency, int auto_p)
{
  theora_info ti;
  theora_state th;
  int result;
  int frame, tframe, keyframe, keydist;
  int shift;
  double rate, ttime;
  yuv_buffer yuv;
  unsigned char *framedata;
  ogg_packet op;
  long long int last_granule = -1;

/*  INFO ("+ Initializing theora_info struct"); */
  theora_info_init (&ti);

  ti.width = 32;
  ti.height = 32;
  ti.frame_width = ti.width;
  ti.frame_height = ti.frame_height;
  ti.offset_x = 0;
  ti.offset_y = 0;
  ti.fps_numerator = 16;
  ti.fps_denominator = 1;
  ti.aspect_numerator = 1;
  ti.aspect_denominator = 1;
  ti.colorspace = OC_CS_UNSPECIFIED;
  ti.pixelformat = OC_PF_420;
  ti.target_bitrate = 0;
  ti.quality = 16;

  ti.dropframes_p = 0;
  ti.quick_p = 1;

  /* check variations of automatic or forced keyframe choice */
  ti.keyframe_auto_p = auto_p;
  /* check with variations of the maximum gap */
  ti.keyframe_frequency = frequency;
  ti.keyframe_frequency_force = frequency;

  ti.keyframe_data_target_bitrate = ti.target_bitrate * 1.5;
  ti.keyframe_auto_threshold = 80;
  ti.keyframe_mindistance = MIN(8, frequency);
  ti.noise_sensitivity = 1;

/*  INFO ("+ Initializing theora_state for encoding"); */
  result = theora_encode_init (&th, &ti);
  if (result == OC_DISABLED) {
    INFO ("+ Clearing theora_state");
    theora_clear (&th);
  } else if (result < 0) {
    FAIL ("negative return code initializing encoder");
  }

/*  INFO ("+ Setting up dummy 4:2:0 frame data"); */
  framedata = calloc(ti.height, ti.width);
  yuv.y_width = ti.width;
  yuv.y_height = ti.height;
  yuv.y_stride = ti.width;
  yuv.y = framedata;
  yuv.uv_width = ti.width / 2;
  yuv.uv_height = ti.width / 2;
  yuv.uv_stride = ti.width;
  yuv.u = framedata;
  yuv.v = framedata;

  INFO ("+ Checking granulepos generation");
  shift = theora_granule_shift(&ti);
  rate = (double)ti.fps_denominator/ti.fps_numerator;
  for (frame = 0; frame < frequency * 2 + 1; frame++) {
    result = theora_encode_YUVin (&th, &yuv);
    if (result < 0) {
      printf("theora_encode_YUVin() returned %d\n", result);
      FAIL ("negative error code submitting frame for compression");
    }
    theora_encode_packetout (&th, frame >= frequency * 2, &op);
    if ((long long int)op.granulepos < last_granule)
      FAIL ("encoder returned a decreasing granulepos value");
    last_granule = op.granulepos;
    keyframe = op.granulepos >> shift;
    keydist = op.granulepos - (keyframe << shift);
    tframe = theora_granule_frame (&th, op.granulepos);
    ttime = theora_granule_time(&th, op.granulepos);
#if DEBUG
    printf("++ frame %d granulepos %lld %d:%d %d %.3lfs\n", 
	frame, (long long int)op.granulepos, keyframe, keydist,
	tframe, theora_granule_time (&th, op.granulepos));
#endif
    if ((keyframe + keydist) != frame + 1)
      FAIL ("encoder granulepos does not map to the correct frame number");
    if (tframe != frame)
      FAIL ("theora_granule_frame returned incorrect results");
    if (fabs(rate*(frame+1) - ttime) > 1.0e-6)
      FAIL ("theora_granule_time returned incorrect results");
  }

  /* clean up */
/*  INFO ("+ Freeing dummy frame data"); */
  free (framedata);

/*  INFO ("+ Clearing theora_info struct"); */
  theora_info_clear (&ti);

/*  INFO ("+ Clearing theora_state"); */
  theora_clear (&th);

  return 0;
}

int main(int argc, char *argv[])
{

  granulepos_test_encode (1, 1);
  granulepos_test_encode (2, 1);
  granulepos_test_encode (3, 1);
  granulepos_test_encode (4, 1);
  granulepos_test_encode (8, 1);
  granulepos_test_encode (64, 1);

  exit (0);
}
