/*****************************************************************************
 * This file is part of Kvazaar HEVC encoder.
 *
 * Copyright (C) 2013-2015 Tampere University of Technology and others (see
 * COPYING file).
 *
 * Kvazaar is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * Kvazaar is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Kvazaar.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/

#include "strategies/altivec/picture-altivec.h"

#if COMPILE_POWERPC_ALTIVEC
#undef bool
#include <altivec.h>
#define bool _Bool
#include <stdlib.h>

#include "kvazaar.h"
#include "strategyselector.h"


static unsigned reg_sad_altivec(const kvz_pixel * const data1, const kvz_pixel * const data2,
                        const int width, const int height, const unsigned stride1, const unsigned stride2)
{
  vector unsigned int vsad = {0,0,0,0}, vzero = {0,0,0,0}; 
  vector signed int sumdiffs;
  int tmpsad, sad = 0;
  
  int y, x;
  
  for (y = 0; y < height; ++y) {
    vector unsigned char perm1, perm2;
    
    perm1 = vec_lvsl(0, &data1[y * stride1]);
    perm2 = vec_lvsl(0, &data2[y * stride2]);
    
    for (x = 0; x <= width-16; x+=16) {
      vector unsigned char t1, t2, t3, t4, t5;
      vector unsigned char *current, *previous;
      
      current = (vector unsigned char *) &data1[y * stride1 + x];
      previous = (vector unsigned char *) &data2[y * stride2 + x];
      
      t1  = vec_perm(current[0], current[1], perm1 );  /* align current vector  */ 
      t2  = vec_perm(previous[0], previous[1], perm2 );/* align previous vector */ 
      t3  = vec_max(t1, t2 );      /* find largest of two           */ 
      t4  = vec_min(t1, t2 );      /* find smaller of two           */ 
      t5  = vec_sub(t3, t4);       /* find absolute difference      */ 
      vsad = vec_sum4s(t5, vsad);    /* accumulate sum of differences */
    }

    for (; x < width; ++x) {
      sad += abs(data1[y * stride1 + x] - data2[y * stride2 + x]);
    }
  }
  
  sumdiffs = vec_sums((vector signed int) vsad, (vector signed int) vzero);
  /* copy vector sum into unaligned result */
  sumdiffs = vec_splat( sumdiffs, 3);
  vec_ste( sumdiffs, 0, &tmpsad );
  sad += tmpsad;
  
  return sad;
}

#endif //COMPILE_POWERPC_ALTIVEC

int kvz_strategy_register_picture_altivec(void* opaque, uint8_t bitdepth) {
  bool success = true;
#if COMPILE_POWERPC_ALTIVEC
  if(bitdepth == 8){
    success &= kvz_strategyselector_register(opaque, "reg_sad", "altivec", 10, &reg_sad_altivec);
  }
#endif
  return success;
}
