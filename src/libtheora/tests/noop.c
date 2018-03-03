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

  function: routines for validating codec initialization
  last mod: $Id: noop.c 16503 2009-08-22 18:14:02Z giles $

 ********************************************************************/

#include <theora/theoraenc.h>
#include <theora/theoradec.h>

#include "tests.h"

static int
noop_test_info ()
{
  th_info ti;

  INFO ("+ Initializing th_info struct");
  th_info_init (&ti);

  INFO ("+ Clearing empty th_info struct");
  th_info_clear (&ti);

  return 0;
}

static int
noop_test_comments ()
{
  th_comment tc;

  INFO ("+ Initializing th_comment struct");
  th_comment_init (&tc);

  INFO ("+ Clearing empty th_comment struct")
  th_comment_clear (&tc);

  return 0;
}

static int
noop_test_encode ()
{
  th_info ti;
  th_enc_ctx *te;

  INFO ("+ Initializing th_info struct");
  th_info_init (&ti);

  INFO ("+ Testing encoder context with empty th_info");
  te = th_encode_alloc(&ti);
  if (te != NULL)
    FAIL("td_encode_alloc accepted an unconfigured th_info");

  INFO ("+ Setting 16x16 image size");
  ti.frame_width = 16;
  ti.frame_height = 16;

  INFO ("+ Allocating encoder context");
  te = th_encode_alloc(&ti);
  if (te == NULL)
    FAIL("td_encode_alloc returned a null pointer");

  INFO ("+ Clearing th_info struct");
  th_info_clear (&ti);

  INFO ("+ Freeing encoder context");
  th_encode_free(te);

  return 0;
}

static int
noop_test_decode ()
{
  th_info ti;
  th_dec_ctx *td;

  INFO ("+ Testing decoder context with null info and setup");
  td = th_decode_alloc(NULL, NULL);
  if (td != NULL)
    FAIL("td_decode_alloc accepted null info pointers");

  INFO ("+ Initializing th_info struct");
  th_info_init (&ti);

  INFO ("+ Testing decoder context with empty info and null setup");
  td = th_decode_alloc(&ti, NULL);
  if (td != NULL)
    FAIL("td_decode_alloc accepted null info pointers");

  INFO ("+ Clearing th_info struct");
  th_info_clear (&ti);

  return 0;
}

int main(int argc, char *argv[])
{
  noop_test_info ();

  noop_test_comments ();

  noop_test_encode ();

  noop_test_decode ();

  exit (0);
}
