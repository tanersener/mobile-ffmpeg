/*
 *  TwoLAME: an optimized MPEG Audio Layer Two encoder
 *
 *  Copyright (C) 2001-2004 Michael Cheng
 *  Copyright (C) 2004-2018 The TwoLAME Project
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#include <stdio.h>
#include <string.h>

#include "twolame.h"
#include "common.h"
#include "dab.h"


void
twolame_dab_crc_calc(twolame_options * glopts,
                     unsigned int bit_alloc[2][SBLIMIT],
                     unsigned int scfsi[2][SBLIMIT],
                     unsigned int scalar[2][3][SBLIMIT], unsigned int *crc, int packed)
{
    int i, j, k;
    int nch = glopts->num_channels_out;
    int nb_scalar;
    int f[5] = { 0, 4, 8, 16, 30 };
    int first, last;

    first = f[packed];
    last = f[packed + 1];
    if (last > glopts->sblimit)
        last = glopts->sblimit;

    nb_scalar = 0;
    *crc = 0x0;
    for (i = first; i < last; i++)
        for (k = 0; k < nch; k++)
            if (bit_alloc[k][i])    /* above jsbound, bit_alloc[0][i] == ba[1][i] */
                switch (scfsi[k][i]) {
                case 0:
                    for (j = 0; j < 3; j++) {
                        nb_scalar++;
                        twolame_dab_crc_update(scalar[k][j][i] >> 3, 3, crc);
                    }
                    break;
                case 1:
                case 3:
                    nb_scalar += 2;
                    twolame_dab_crc_update(scalar[k][0][i] >> 3, 3, crc);
                    twolame_dab_crc_update(scalar[k][2][i] >> 3, 3, crc);
                    break;
                case 2:
                    nb_scalar++;
                    twolame_dab_crc_update(scalar[k][0][i] >> 3, 3, crc);
                }
}

void twolame_dab_crc_update(unsigned int data, unsigned int length, unsigned int *crc)
{
    unsigned int masking, carry;

    masking = 1 << length;

    while ((masking >>= 1)) {
        carry = *crc & 0x80;
        *crc <<= 1;
        if (!carry ^ !(data & masking))
            *crc ^= CRC8_POLYNOMIAL;
    }
    *crc &= 0xff;
}

// vim:ts=4:sw=4:nowrap:
