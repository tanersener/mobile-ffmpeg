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

  function: common test utilities
  last mod: $Id: tests.h 16503 2009-08-22 18:14:02Z giles $

 ********************************************************************/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#define INFO(str) \
  { printf ("----  %s ...\n", (str)); }

#define WARN(str) \
  { printf ("%s:%d: warning: %s\n", __FILE__, __LINE__, (str)); }

#define FAIL(str) \
  { printf ("%s:%d: %s\n", __FILE__, __LINE__, (str)); exit(1); }

#undef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
