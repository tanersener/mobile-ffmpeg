/*
 *	time status related function include file
 *
 *	Copyright (c) 1999 Mark Taylor
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef LAME_TIMESTATUS_H
#define LAME_TIMESTATUS_H

#if defined(__cplusplus)
extern "C" {
#endif

void    encoder_progress_begin( lame_global_flags const*    gfp
                              , char const*                 inpath
                              , char const*                 outpath );
void    encoder_progress( lame_global_flags const* gfp );
void    encoder_progress_end(lame_global_flags const* gfp);

struct DecoderProgress;
typedef struct DecoderProgress* DecoderProgress;

DecoderProgress decoder_progress_init(unsigned long n, int framesize);
void    decoder_progress(DecoderProgress dp, const mp3data_struct *, int iread);
void    decoder_progress_finish(DecoderProgress dp);

#if defined(__cplusplus)
}
#endif

#endif /* LAME_TIMESTATUS_H */
