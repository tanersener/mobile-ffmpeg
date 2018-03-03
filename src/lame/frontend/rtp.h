/*
 *      rtp socket communication include file
 *
 *      initially contributed by Felix von Leitner
 *
 *      Copyright (c) 2000 Mark Taylor
 *                    2010 Robert Hegemann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef LAME_RTP_H
#define LAME_RTP_H

#if defined(__cplusplus)
extern  "C" {
#endif

    struct RtpStruct;
    typedef struct RtpStruct *RtpHandle;

    void    rtp_initialization(void);
    void    rtp_deinitialization(void);
    int     rtp_socket( /*RtpHandle rtp, */ char const *Address, unsigned int port,
                       unsigned int TTL);
    void    rtp_output( /*RtpHandle rtp, */ unsigned char const *mp3buffer, int mp3size);

#if defined(__cplusplus)
}
#endif
#endif
