/*
 *  avilib.c
 *
 *  Copyright (C) Thomas Östreich - June 2001
 *  multiple audio track support Copyright (C) 2002 Thomas Östreich 
 *
 *  Original code:
 *  Copyright (C) 1999 Rainer Johanni <Rainer@Johanni.de> 
 *
 *  This file is part of transcode, a linux video stream processing tool
 *      
 *  transcode is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  transcode is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

#include "avilib.h"
//#include <time.h>

#define INFO_LIST

/* The following variable indicates the kind of error */

long AVI_errno;

#define MAX_INFO_STRLEN 64
static char id_str[MAX_INFO_STRLEN];

#define FRAME_RATE_SCALE 1000000

#ifndef PACKAGE
#define PACKAGE "my"
#define VERSION "0.00"
#endif

#ifndef O_BINARY
/* win32 wants a binary flag to open(); this sets it to null
   on platforms that don't have it. */
#define O_BINARY 0
#endif

/*******************************************************************
 *                                                                 *
 *    Utilities for writing an AVI File                            *
 *                                                                 *
 *******************************************************************/

static size_t avi_read(int fd, char *buf, size_t len)
{
   size_t n = 0;
   size_t r = 0;

   while (r < len) {
      n = read (fd, buf + r, len - r);

      if (n <= 0)
	  return r;
      r += n;
   }

   return r;
}

static size_t avi_write (int fd, char *buf, size_t len)
{
   size_t n = 0;
   size_t r = 0;

   while (r < len) {
      n = write (fd, buf + r, len - r);
      if (n < 0)
         return n;
      
      r += n;
   }
   return r;
}

/* HEADERBYTES: The number of bytes to reserve for the header */

#define HEADERBYTES 2048

/* AVI_MAX_LEN: The maximum length of an AVI file, we stay a bit below
    the 2GB limit (Remember: 2*10^9 is smaller than 2 GB) */

#define AVI_MAX_LEN (UINT_MAX-(1<<20)*16-HEADERBYTES)

#define PAD_EVEN(x) ( ((x)+1) & ~1 )


/* Copy n into dst as a 4 byte, little endian number.
   Should also work on big endian machines */

static void long2str(unsigned char *dst, int n)
{
   dst[0] = (n    )&0xff;
   dst[1] = (n>> 8)&0xff;
   dst[2] = (n>>16)&0xff;
   dst[3] = (n>>24)&0xff;
}

/* Convert a string of 4 or 2 bytes to a number,
   also working on big endian machines */

static unsigned long str2ulong(unsigned char *str)
{
   return ( str[0] | (str[1]<<8) | (str[2]<<16) | (str[3]<<24) );
}
static unsigned long str2ushort(unsigned char *str)
{
   return ( str[0] | (str[1]<<8) );
}

/* Calculate audio sample size from number of bits and number of channels.
   This may have to be adjusted for eg. 12 bits and stereo */

static int avi_sampsize(avi_t *AVI, int j)
{
   int s;
   s = ((AVI->track[j].a_bits+7)/8)*AVI->track[j].a_chans;
   //   if(s==0) s=1; /* avoid possible zero divisions */
   if(s<4) s=4; /* avoid possible zero divisions */ 
   return s;
}

/* Add a chunk (=tag and data) to the AVI file,
   returns -1 on write error, 0 on success */

static int avi_add_chunk(avi_t *AVI, unsigned char *tag, unsigned char *data, int length)
{
   unsigned char c[8];

   /* Copy tag and length int c, so that we need only 1 write system call
      for these two values */

   memcpy(c,tag,4);
   long2str(c+4,length);

   /* Output tag, length and data, restore previous position
      if the write fails */

   length = PAD_EVEN(length);

   if( avi_write(AVI->fdes,(char *)c,8) != 8 ||
       avi_write(AVI->fdes,(char *)data,length) != length )
   {
      lseek(AVI->fdes,AVI->pos,SEEK_SET);
      AVI_errno = AVI_ERR_WRITE;
      return -1;
   }

   /* Update file position */

   AVI->pos += 8 + length;

   //fprintf(stderr, "pos=%lu %s\n", AVI->pos, tag);

   return 0;
}

static int avi_add_index_entry(avi_t *AVI, unsigned char *tag, long flags, unsigned long pos, unsigned long len)
{
   void *ptr;

   if(AVI->n_idx>=AVI->max_idx) {
     ptr = realloc((void *)AVI->idx,(AVI->max_idx+4096)*16);
     
     if(ptr == 0) {
       AVI_errno = AVI_ERR_NO_MEM;
       return -1;
     }
     AVI->max_idx += 4096;
     AVI->idx = (unsigned char((*)[16]) ) ptr;
   }
   
   /* Add index entry */

   //   fprintf(stderr, "INDEX %s %ld %lu %lu\n", tag, flags, pos, len);

   memcpy(AVI->idx[AVI->n_idx],tag,4);
   long2str(AVI->idx[AVI->n_idx]+ 4,flags);
   long2str(AVI->idx[AVI->n_idx]+ 8, pos);
   long2str(AVI->idx[AVI->n_idx]+12, len);
   
   /* Update counter */

   AVI->n_idx++;

   if(len>AVI->max_len) AVI->max_len=len;

   return 0;
}

/*
   AVI_open_output_file: Open an AVI File and write a bunch
                         of zero bytes as space for the header.

   returns a pointer to avi_t on success, a zero pointer on error
*/

avi_t* AVI_open_output_file(char * filename)
{
   avi_t *AVI;
   int i;

   int mask = 0;
   
   unsigned char AVI_header[HEADERBYTES];

   /* Allocate the avi_t struct and zero it */

   AVI = (avi_t *) malloc(sizeof(avi_t));
   if(AVI==0)
   {
      AVI_errno = AVI_ERR_NO_MEM;
      return 0;
   }
   memset((void *)AVI,0,sizeof(avi_t));

   /* Since Linux needs a long time when deleting big files,
      we do not truncate the file when we open it.
      Instead it is truncated when the AVI file is closed */

  /* mask = umask (0);
   umask (mask);*/

   AVI->fdes = open(filename, O_RDWR|O_CREAT|O_BINARY, 0644 &~ mask);
   if (AVI->fdes < 0)
   {
      AVI_errno = AVI_ERR_OPEN;
      free(AVI);
      return 0;
   }

   /* Write out HEADERBYTES bytes, the header will go here
      when we are finished with writing */

   for (i=0;i<HEADERBYTES;i++) AVI_header[i] = 0;
   i = avi_write(AVI->fdes,(char *)AVI_header,HEADERBYTES);
   if (i != HEADERBYTES)
   {
      close(AVI->fdes);
      AVI_errno = AVI_ERR_WRITE;
      free(AVI);
      return 0;
   }

   AVI->pos  = HEADERBYTES;
   AVI->mode = AVI_MODE_WRITE; /* open for writing */

   //init
   AVI->anum = 0;
   AVI->aptr = 0;

   return AVI;
}

void AVI_set_video(avi_t *AVI, int width, int height, double fps, char *compressor)
{
   /* may only be called if file is open for writing */

   if(AVI->mode==AVI_MODE_READ) return;

   AVI->width  = width;
   AVI->height = height;
   AVI->fps    = fps;
   
   if(strncmp(compressor, "RGB", 3)==0) {
     memset(AVI->compressor, 0, 4);
   } else {
     memcpy(AVI->compressor,compressor,4);
   }     
   
   AVI->compressor[4] = 0;

   avi_update_header(AVI);
}

void AVI_set_audio(avi_t *AVI, int channels, long rate, int bits, int format, long mp3rate)
{
   /* may only be called if file is open for writing */

   if(AVI->mode==AVI_MODE_READ) return;

   //inc audio tracks
   AVI->aptr=AVI->anum;
   ++AVI->anum;

   if(AVI->anum > AVI_MAX_TRACKS) {
     fprintf(stderr, "error - only %d audio tracks supported\n", AVI_MAX_TRACKS);
     exit(1);
   }

   AVI->track[AVI->aptr].a_chans = channels;
   AVI->track[AVI->aptr].a_rate  = rate;
   AVI->track[AVI->aptr].a_bits  = bits;
   AVI->track[AVI->aptr].a_fmt   = format;
   AVI->track[AVI->aptr].mp3rate = mp3rate;

   avi_update_header(AVI);
}

#define OUT4CC(s) \
   if(nhb<=HEADERBYTES-4) memcpy(AVI_header+nhb,s,4); nhb += 4

#define OUTLONG(n) \
   if(nhb<=HEADERBYTES-4) long2str(AVI_header+nhb,n); nhb += 4

#define OUTSHRT(n) \
   if(nhb<=HEADERBYTES-2) { \
      AVI_header[nhb  ] = (n   )&0xff; \
      AVI_header[nhb+1] = (n>>8)&0xff; \
   } \
   nhb += 2


//ThOe write preliminary AVI file header: 0 frames, max vid/aud size
int avi_update_header(avi_t *AVI)
{
   int njunk, sampsize, hasIndex, ms_per_frame, frate, flag;
   int movi_len, hdrl_start, strl_start, j;
   unsigned char AVI_header[HEADERBYTES];
   long nhb;

   //assume max size
   movi_len = AVI_MAX_LEN - HEADERBYTES + 4;

   //assume index will be written
   hasIndex=1;

   if(AVI->fps < 0.001) {
     frate=0;
     ms_per_frame=0;
   } else {
     frate = (int) (FRAME_RATE_SCALE*AVI->fps + 0.5);
     ms_per_frame=(int) (1000000/AVI->fps + 0.5);
   }

   /* Prepare the file header */

   nhb = 0;

   /* The RIFF header */

   OUT4CC ("RIFF");
   OUTLONG(movi_len);    // assume max size
   OUT4CC ("AVI ");

   /* Start the header list */

   OUT4CC ("LIST");
   OUTLONG(0);        /* Length of list in bytes, don't know yet */
   hdrl_start = nhb;  /* Store start position */
   OUT4CC ("hdrl");

   /* The main AVI header */

   /* The Flags in AVI File header */

#define AVIF_HASINDEX           0x00000010      /* Index at end of file */
#define AVIF_MUSTUSEINDEX       0x00000020
#define AVIF_ISINTERLEAVED      0x00000100
#define AVIF_TRUSTCKTYPE        0x00000800      /* Use CKType to find key frames */
#define AVIF_WASCAPTUREFILE     0x00010000
#define AVIF_COPYRIGHTED        0x00020000

   OUT4CC ("avih");
   OUTLONG(56);                 /* # of bytes to follow */
   OUTLONG(ms_per_frame);       /* Microseconds per frame */
   //ThOe ->0 
   //   OUTLONG(10000000);           /* MaxBytesPerSec, I hope this will never be used */
   OUTLONG(0);
   OUTLONG(0);                  /* PaddingGranularity (whatever that might be) */
                                /* Other sources call it 'reserved' */
   flag = AVIF_ISINTERLEAVED;
   if(hasIndex) flag |= AVIF_HASINDEX;
   if(hasIndex && AVI->must_use_index) flag |= AVIF_MUSTUSEINDEX;
   OUTLONG(flag);               /* Flags */
   OUTLONG(0);                  // no frames yet
   OUTLONG(0);                  /* InitialFrames */

   OUTLONG(AVI->anum+1);

   OUTLONG(0);                  /* SuggestedBufferSize */
   OUTLONG(AVI->width);         /* Width */
   OUTLONG(AVI->height);        /* Height */
                                /* MS calls the following 'reserved': */
   OUTLONG(0);                  /* TimeScale:  Unit used to measure time */
   OUTLONG(0);                  /* DataRate:   Data rate of playback     */
   OUTLONG(0);                  /* StartTime:  Starting time of AVI data */
   OUTLONG(0);                  /* DataLength: Size of AVI data chunk    */


   /* Start the video stream list ---------------------------------- */

   OUT4CC ("LIST");
   OUTLONG(0);        /* Length of list in bytes, don't know yet */
   strl_start = nhb;  /* Store start position */
   OUT4CC ("strl");

   /* The video stream header */

   OUT4CC ("strh");
   OUTLONG(56);                 /* # of bytes to follow */
   OUT4CC ("vids");             /* Type */
   OUT4CC (AVI->compressor);    /* Handler */
   OUTLONG(0);                  /* Flags */
   OUTLONG(0);                  /* Reserved, MS says: wPriority, wLanguage */
   OUTLONG(0);                  /* InitialFrames */
   OUTLONG(FRAME_RATE_SCALE);              /* Scale */
   OUTLONG(frate);              /* Rate: Rate/Scale == samples/second */
   OUTLONG(0);                  /* Start */
   OUTLONG(0);                  // no frames yet
   OUTLONG(0);                  /* SuggestedBufferSize */
   OUTLONG(-1);                 /* Quality */
   OUTLONG(0);                  /* SampleSize */
   OUTLONG(0);                  /* Frame */
   OUTLONG(0);                  /* Frame */
   //   OUTLONG(0);                  /* Frame */
   //OUTLONG(0);                  /* Frame */

   /* The video stream format */

   OUT4CC ("strf");
   OUTLONG(40);                 /* # of bytes to follow */
   OUTLONG(40);                 /* Size */
   OUTLONG(AVI->width);         /* Width */
   OUTLONG(AVI->height);        /* Height */
   OUTSHRT(1); OUTSHRT(24);     /* Planes, Count */
   OUT4CC (AVI->compressor);    /* Compression */
   // ThOe (*3)
   OUTLONG(AVI->width*AVI->height*3);  /* SizeImage (in bytes?) */
   OUTLONG(0);                  /* XPelsPerMeter */
   OUTLONG(0);                  /* YPelsPerMeter */
   OUTLONG(0);                  /* ClrUsed: Number of colors used */
   OUTLONG(0);                  /* ClrImportant: Number of colors important */

   /* Finish stream list, i.e. put number of bytes in the list to proper pos */

   long2str(AVI_header+strl_start-4,nhb-strl_start);

   
   /* Start the audio stream list ---------------------------------- */
   
   for(j=0; j<AVI->anum; ++j) {
       
       sampsize = avi_sampsize(AVI, j);
   
       OUT4CC ("LIST");
       OUTLONG(0);        /* Length of list in bytes, don't know yet */
       strl_start = nhb;  /* Store start position */
       OUT4CC ("strl");
       
       /* The audio stream header */
       
       OUT4CC ("strh");
       OUTLONG(56);            /* # of bytes to follow */
       OUT4CC ("auds");
       
       // -----------
       // ThOe
       OUTLONG(0);             /* Format (Optionally) */
       // -----------
       
       OUTLONG(0);             /* Flags */
       OUTLONG(0);             /* Reserved, MS says: wPriority, wLanguage */
       OUTLONG(0);             /* InitialFrames */
       
       // ThOe /4
       OUTLONG(sampsize/4);      /* Scale */
       OUTLONG(1000*AVI->track[j].mp3rate/8);
       OUTLONG(0);             /* Start */
       OUTLONG(4*AVI->track[j].audio_bytes/sampsize);   /* Length */
       OUTLONG(0);             /* SuggestedBufferSize */
       OUTLONG(-1);            /* Quality */
       
       // ThOe /4
       OUTLONG(sampsize/4);    /* SampleSize */
       
       OUTLONG(0);             /* Frame */
       OUTLONG(0);             /* Frame */
       //       OUTLONG(0);             /* Frame */
       //OUTLONG(0);             /* Frame */
       
       /* The audio stream format */
       
       OUT4CC ("strf");
       OUTLONG(16);                   /* # of bytes to follow */
       OUTSHRT(AVI->track[j].a_fmt);           /* Format */
       OUTSHRT(AVI->track[j].a_chans);         /* Number of channels */
       OUTLONG(AVI->track[j].a_rate);          /* SamplesPerSec */
       // ThOe
       OUTLONG(1000*AVI->track[j].mp3rate/8);
       //ThOe (/4)
       
       OUTSHRT(sampsize/4);           /* BlockAlign */
       
       
       OUTSHRT(AVI->track[j].a_bits);          /* BitsPerSample */
       
       /* Finish stream list, i.e. put number of bytes in the list to proper pos */
       
       long2str(AVI_header+strl_start-4,nhb-strl_start);
   }
   
   /* Finish header list */
   
   long2str(AVI_header+hdrl_start-4,nhb-hdrl_start);
   
   
   /* Calculate the needed amount of junk bytes, output junk */
   
   njunk = HEADERBYTES - nhb - 8 - 12;
   
   /* Safety first: if njunk <= 0, somebody has played with
      HEADERBYTES without knowing what (s)he did.
      This is a fatal error */
   
   if(njunk<=0)
     {
       fprintf(stderr,"AVI_close_output_file: # of header bytes too small\n");
       exit(1);
     }
   
   OUT4CC ("JUNK");
   OUTLONG(njunk);
   memset(AVI_header+nhb,0,njunk);
   
   //11/14/01 added id string 

   if(njunk > strlen(id_str)+8) {
     sprintf(id_str, "%s-%s", PACKAGE, VERSION);
     memcpy(AVI_header+nhb, id_str, strlen(id_str));
   }
   
   nhb += njunk;

   /* Start the movi list */

   OUT4CC ("LIST");
   OUTLONG(movi_len); /* Length of list in bytes */
   OUT4CC ("movi");

   /* Output the header, truncate the file to the number of bytes
      actually written, report an error if someting goes wrong */

   if ( lseek(AVI->fdes,0,SEEK_SET)<0 ||
        avi_write(AVI->fdes,(char *)AVI_header,HEADERBYTES)!=HEADERBYTES ||
	lseek(AVI->fdes,AVI->pos,SEEK_SET)<0)
     {
       AVI_errno = AVI_ERR_CLOSE;
       return -1;
     }

   return 0;
}

/*
  Write the header of an AVI file and close it.
  returns 0 on success, -1 on write error.
*/

static int avi_close_output_file(avi_t *AVI)
{

   int ret, njunk, sampsize, hasIndex, ms_per_frame, frate, idxerror, flag;
   unsigned long movi_len;
   int hdrl_start, strl_start, j;
   unsigned char AVI_header[HEADERBYTES];
   long nhb;

#ifdef INFO_LIST
   long info_len;
//   time_t calptr;
#endif

   /* Calculate length of movi list */

   movi_len = AVI->pos - HEADERBYTES + 4;

   /* Try to ouput the index entries. This may fail e.g. if no space
      is left on device. We will report this as an error, but we still
      try to write the header correctly (so that the file still may be
      readable in the most cases */

   idxerror = 0;
   //   fprintf(stderr, "pos=%lu, index_len=%ld             \n", AVI->pos, AVI->n_idx*16);
   ret = avi_add_chunk(AVI, (unsigned char *)"idx1", (void*)AVI->idx, AVI->n_idx*16);
   hasIndex = (ret==0);
   //fprintf(stderr, "pos=%lu, index_len=%d\n", AVI->pos, hasIndex);

   if(ret) {
     idxerror = 1;
     AVI_errno = AVI_ERR_WRITE_INDEX;
   }
   
   /* Calculate Microseconds per frame */

   if(AVI->fps < 0.001) {
     frate=0;
     ms_per_frame=0;
   } else {
     frate = (int) (FRAME_RATE_SCALE*AVI->fps + 0.5);
     ms_per_frame=(int) (1000000/AVI->fps + 0.5);
   }

   /* Prepare the file header */

   nhb = 0;

   /* The RIFF header */

   OUT4CC ("RIFF");
   OUTLONG(AVI->pos - 8);    /* # of bytes to follow */
   OUT4CC ("AVI ");

   /* Start the header list */

   OUT4CC ("LIST");
   OUTLONG(0);        /* Length of list in bytes, don't know yet */
   hdrl_start = nhb;  /* Store start position */
   OUT4CC ("hdrl");

   /* The main AVI header */

   /* The Flags in AVI File header */

#define AVIF_HASINDEX           0x00000010      /* Index at end of file */
#define AVIF_MUSTUSEINDEX       0x00000020
#define AVIF_ISINTERLEAVED      0x00000100
#define AVIF_TRUSTCKTYPE        0x00000800      /* Use CKType to find key frames */
#define AVIF_WASCAPTUREFILE     0x00010000
#define AVIF_COPYRIGHTED        0x00020000

   OUT4CC ("avih");
   OUTLONG(56);                 /* # of bytes to follow */
   OUTLONG(ms_per_frame);       /* Microseconds per frame */
   //ThOe ->0 
   //   OUTLONG(10000000);           /* MaxBytesPerSec, I hope this will never be used */
   OUTLONG(0);
   OUTLONG(0);                  /* PaddingGranularity (whatever that might be) */
                                /* Other sources call it 'reserved' */
   flag = AVIF_ISINTERLEAVED;
   if(hasIndex) flag |= AVIF_HASINDEX;
   if(hasIndex && AVI->must_use_index) flag |= AVIF_MUSTUSEINDEX;
   OUTLONG(flag);               /* Flags */
   OUTLONG(AVI->video_frames);  /* TotalFrames */
   OUTLONG(0);                  /* InitialFrames */

   OUTLONG(AVI->anum+1);
//   if (AVI->track[0].audio_bytes)
//      { OUTLONG(2); }           /* Streams */
//   else
//      { OUTLONG(1); }           /* Streams */

   OUTLONG(0);                  /* SuggestedBufferSize */
   OUTLONG(AVI->width);         /* Width */
   OUTLONG(AVI->height);        /* Height */
                                /* MS calls the following 'reserved': */
   OUTLONG(0);                  /* TimeScale:  Unit used to measure time */
   OUTLONG(0);                  /* DataRate:   Data rate of playback     */
   OUTLONG(0);                  /* StartTime:  Starting time of AVI data */
   OUTLONG(0);                  /* DataLength: Size of AVI data chunk    */


   /* Start the video stream list ---------------------------------- */

   OUT4CC ("LIST");
   OUTLONG(0);        /* Length of list in bytes, don't know yet */
   strl_start = nhb;  /* Store start position */
   OUT4CC ("strl");

   /* The video stream header */

   OUT4CC ("strh");
   OUTLONG(56);                 /* # of bytes to follow */
   OUT4CC ("vids");             /* Type */
   OUT4CC (AVI->compressor);    /* Handler */
   OUTLONG(0);                  /* Flags */
   OUTLONG(0);                  /* Reserved, MS says: wPriority, wLanguage */
   OUTLONG(0);                  /* InitialFrames */
   OUTLONG(FRAME_RATE_SCALE);              /* Scale */
   OUTLONG(frate);              /* Rate: Rate/Scale == samples/second */
   OUTLONG(0);                  /* Start */
   OUTLONG(AVI->video_frames);  /* Length */
   OUTLONG(0);                  /* SuggestedBufferSize */
   OUTLONG(-1);                 /* Quality */
   OUTLONG(0);                  /* SampleSize */
   OUTLONG(0);                  /* Frame */
   OUTLONG(0);                  /* Frame */
   //   OUTLONG(0);                  /* Frame */
   //OUTLONG(0);                  /* Frame */

   /* The video stream format */

   OUT4CC ("strf");
   OUTLONG(40);                 /* # of bytes to follow */
   OUTLONG(40);                 /* Size */
   OUTLONG(AVI->width);         /* Width */
   OUTLONG(AVI->height);        /* Height */
   OUTSHRT(1); OUTSHRT(24);     /* Planes, Count */
   OUT4CC (AVI->compressor);    /* Compression */
   // ThOe (*3)
   OUTLONG(AVI->width*AVI->height*3);  /* SizeImage (in bytes?) */
   OUTLONG(0);                  /* XPelsPerMeter */
   OUTLONG(0);                  /* YPelsPerMeter */
   OUTLONG(0);                  /* ClrUsed: Number of colors used */
   OUTLONG(0);                  /* ClrImportant: Number of colors important */

   /* Finish stream list, i.e. put number of bytes in the list to proper pos */

   long2str(AVI_header+strl_start-4,nhb-strl_start);

   /* Start the audio stream list ---------------------------------- */

   for(j=0; j<AVI->anum; ++j) {
     
     //if (AVI->track[j].a_chans && AVI->track[j].audio_bytes)
       {
	   
	 sampsize = avi_sampsize(AVI, j);
	   
	 OUT4CC ("LIST");
	 OUTLONG(0);        /* Length of list in bytes, don't know yet */
	 strl_start = nhb;  /* Store start position */
	 OUT4CC ("strl");
	   
	 /* The audio stream header */
	 
	 OUT4CC ("strh");
	 OUTLONG(56);            /* # of bytes to follow */
	 OUT4CC ("auds");
	 
	 // -----------
	 // ThOe
	 OUTLONG(0);             /* Format (Optionally) */
	   // -----------
	   
	 OUTLONG(0);             /* Flags */
	 OUTLONG(0);             /* Reserved, MS says: wPriority, wLanguage */
	 OUTLONG(0);             /* InitialFrames */
	   
	 // ThOe /4
	 OUTLONG(sampsize/4);      /* Scale */
	 OUTLONG(1000*AVI->track[j].mp3rate/8);
	 OUTLONG(0);             /* Start */
	 OUTLONG(4*AVI->track[j].audio_bytes/sampsize);   /* Length */
	 OUTLONG(0);             /* SuggestedBufferSize */
	 OUTLONG(-1);            /* Quality */
	   
	 // ThOe /4
	 OUTLONG(sampsize/4);    /* SampleSize */
	   
	 OUTLONG(0);             /* Frame */
	 OUTLONG(0);             /* Frame */
	 //	 OUTLONG(0);             /* Frame */
	 //OUTLONG(0);             /* Frame */
	   
	 /* The audio stream format */
	 
	 OUT4CC ("strf");
	 OUTLONG(16);                   /* # of bytes to follow */
	 OUTSHRT(AVI->track[j].a_fmt);           /* Format */
	 OUTSHRT(AVI->track[j].a_chans);         /* Number of channels */
	 OUTLONG(AVI->track[j].a_rate);          /* SamplesPerSec */
	 // ThOe
	 OUTLONG(1000*AVI->track[j].mp3rate/8);
	 //ThOe (/4)
	 
	 OUTSHRT(sampsize/4);           /* BlockAlign */
	 
	 
	 OUTSHRT(AVI->track[j].a_bits);          /* BitsPerSample */
	 
	 /* Finish stream list, i.e. put number of bytes in the list to proper pos */
       }
       long2str(AVI_header+strl_start-4,nhb-strl_start);
   }
   
   /* Finish header list */
   
   long2str(AVI_header+hdrl_start-4,nhb-hdrl_start);


   // add INFO list --- (0.6.0pre4)

#ifdef INFO_LIST
   OUT4CC ("LIST");
   
   //FIXME
   info_len = MAX_INFO_STRLEN + 12;
   OUTLONG(info_len);
   OUT4CC ("INFO");

//   OUT4CC ("INAM");
//   OUTLONG(MAX_INFO_STRLEN);

//   sprintf(id_str, "\t");
//   memset(AVI_header+nhb, 0, MAX_INFO_STRLEN);
//   memcpy(AVI_header+nhb, id_str, strlen(id_str));
//   nhb += MAX_INFO_STRLEN;

   OUT4CC ("ISFT");
   OUTLONG(MAX_INFO_STRLEN);

   sprintf(id_str, "%s-%s", PACKAGE, VERSION);
   memset(AVI_header+nhb, 0, MAX_INFO_STRLEN);
   memcpy(AVI_header+nhb, id_str, strlen(id_str));
   nhb += MAX_INFO_STRLEN;

//   OUT4CC ("ICMT");
//   OUTLONG(MAX_INFO_STRLEN);

//   calptr=time(NULL); 
//   sprintf(id_str, "\t%s %s", ctime(&calptr), "");
//   memset(AVI_header+nhb, 0, MAX_INFO_STRLEN);
//   memcpy(AVI_header+nhb, id_str, 25);
//   nhb += MAX_INFO_STRLEN;
#endif

   // ----------------------------
   
   /* Calculate the needed amount of junk bytes, output junk */
   
   njunk = HEADERBYTES - nhb - 8 - 12;
   
   /* Safety first: if njunk <= 0, somebody has played with
      HEADERBYTES without knowing what (s)he did.
      This is a fatal error */
   
   if(njunk<=0)
   {
      fprintf(stderr,"AVI_close_output_file: # of header bytes too small\n");
      exit(1);
   }

   OUT4CC ("JUNK");
   OUTLONG(njunk);
   memset(AVI_header+nhb,0,njunk);
   
   nhb += njunk;

   /* Start the movi list */

   OUT4CC ("LIST");
   OUTLONG(movi_len); /* Length of list in bytes */
   OUT4CC ("movi");

   /* Output the header, truncate the file to the number of bytes
      actually written, report an error if someting goes wrong */

   if ( lseek(AVI->fdes,0,SEEK_SET)<0 ||
        avi_write(AVI->fdes,(char *)AVI_header,HEADERBYTES)!=HEADERBYTES 
        //|| ftruncate(AVI->fdes,AVI->pos)<0 
        )
   {
      AVI_errno = AVI_ERR_CLOSE;
      return -1;
   }

   if(idxerror) return -1;

   return 0;
}

/*
   AVI_write_data:
   Add video or audio data to the file;

   Return values:
    0    No error;
   -1    Error, AVI_errno is set appropriatly;

*/

static int avi_write_data(avi_t *AVI, char *data, unsigned long length, int audio, int keyframe)
{
   int n;

   unsigned char astr[5];

   /* Check for maximum file length */
   
   if ( (AVI->pos + 8 + length + 8 + (AVI->n_idx+1)*16) > AVI_MAX_LEN ) {
     AVI_errno = AVI_ERR_SIZELIM;
     return -1;
   }
   
   /* Add index entry */

   //set tag for current audio track
   sprintf((char *)astr, "0%1dwb", AVI->aptr+1);

   if(audio)
     n = avi_add_index_entry(AVI,astr,0x00,AVI->pos,length);
   else
     n = avi_add_index_entry(AVI,(unsigned char *) "00db",((keyframe)?0x10:0x0),AVI->pos,length);
   
   if(n) return -1;
   
   /* Output tag and data */
   
   if(audio)
     n = avi_add_chunk(AVI,(unsigned char *) astr, (unsigned char *)data,length);
   else
     n = avi_add_chunk(AVI,(unsigned char *)"00db",(unsigned char *)data,length);
   
   if (n) return -1;
   
   return 0;
}

int AVI_write_frame(avi_t *AVI, char *data, long bytes, int keyframe)
{
  unsigned long pos;
  
  if(AVI->mode==AVI_MODE_READ) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
  
  pos = AVI->pos;

  if(avi_write_data(AVI,data,bytes,0,keyframe)) return -1;
   
  AVI->last_pos = pos;
  AVI->last_len = bytes;
  AVI->video_frames++;
  return 0;
}

int AVI_dup_frame(avi_t *AVI)
{
   if(AVI->mode==AVI_MODE_READ) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }

   if(AVI->last_pos==0) return 0; /* No previous real frame */
   if(avi_add_index_entry(AVI,(unsigned char *)"00db",0x10,AVI->last_pos,AVI->last_len)) return -1;
   AVI->video_frames++;
   AVI->must_use_index = 1;
   return 0;
}

int AVI_write_audio(avi_t *AVI, char *data, long bytes)
{
   if(AVI->mode==AVI_MODE_READ) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }

   if( avi_write_data(AVI,data,bytes,1,0) ) return -1;
   AVI->track[AVI->aptr].audio_bytes += bytes;
   return 0;
}


int AVI_append_audio(avi_t *AVI, char *data, long bytes)
{

  long i, length, pos;
  unsigned char c[4];

  if(AVI->mode==AVI_MODE_READ) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
  
  // update last index entry:
  
  --AVI->n_idx;
  length = str2ulong(AVI->idx[AVI->n_idx]+12);
  pos    = str2ulong(AVI->idx[AVI->n_idx]+8);

  //update;
  long2str(AVI->idx[AVI->n_idx]+12,length+bytes);   

  ++AVI->n_idx;

  AVI->track[AVI->aptr].audio_bytes += bytes;

  //update chunk header
  lseek(AVI->fdes, pos+4, SEEK_SET);
  long2str(c, length+bytes);     
  avi_write(AVI->fdes,(char *) c, 4);

  lseek(AVI->fdes, pos+8+length, SEEK_SET);

  i=PAD_EVEN(length + bytes);

  bytes = i - length;
  avi_write(AVI->fdes, data, bytes);
  AVI->pos = pos + 8 + i;

  return 0;
}


long AVI_bytes_remain(avi_t *AVI)
{
   if(AVI->mode==AVI_MODE_READ) return 0;

   return ( AVI_MAX_LEN - (AVI->pos + 8 + 16*AVI->n_idx));
}

long AVI_bytes_written(avi_t *AVI)
{
   if(AVI->mode==AVI_MODE_READ) return 0;

   return (AVI->pos + 8 + 16*AVI->n_idx);
}

int AVI_set_audio_track(avi_t *AVI, int track)
{
  
  if(track < 0 || track + 1 > AVI->anum) return(-1);

  //this info is not written to file anyway
  AVI->aptr=track;
  return 0;
}

int AVI_get_audio_track(avi_t *AVI)
{
    return(AVI->aptr);
}


/*******************************************************************
 *                                                                 *
 *    Utilities for reading video and audio from an AVI File       *
 *                                                                 *
 *******************************************************************/

int AVI_close(avi_t *AVI)
{
   int ret;

   /* If the file was open for writing, the header and index still have
      to be written */

   if(AVI->mode == AVI_MODE_WRITE)
      ret = avi_close_output_file(AVI);
   else
      ret = 0;

   /* Even if there happened an error, we first clean up */

   close(AVI->fdes);
   if(AVI->idx) free(AVI->idx);
   if(AVI->video_index) free(AVI->video_index);
   //FIXME
   //if(AVI->audio_index) free(AVI->audio_index);
   free(AVI);

   return ret;
}


#define ERR_EXIT(x) \
{ \
   AVI_close(AVI); \
   AVI_errno = x; \
   return 0; \
}

avi_t *AVI_open_input_file(char *filename, int getIndex)
{
  avi_t *AVI=NULL;
  
  /* Create avi_t structure */
  
  AVI = (avi_t *) malloc(sizeof(avi_t));
  if(AVI==NULL)
    {
      AVI_errno = AVI_ERR_NO_MEM;
      return 0;
    }
  memset((void *)AVI,0,sizeof(avi_t));
  
  AVI->mode = AVI_MODE_READ; /* open for reading */
  
  /* Open the file */
  
  AVI->fdes = open(filename,O_RDONLY|O_BINARY);
  if(AVI->fdes < 0)
    {
      AVI_errno = AVI_ERR_OPEN;
      free(AVI);
      return 0;
    }
  
  avi_parse_input_file(AVI, getIndex);

  AVI->aptr=0; //reset  

  return AVI;
}

avi_t *AVI_open_fd(int fd, int getIndex)
{
  avi_t *AVI=NULL;
  
  /* Create avi_t structure */
  
  AVI = (avi_t *) malloc(sizeof(avi_t));
  if(AVI==NULL)
    {
      AVI_errno = AVI_ERR_NO_MEM;
      return 0;
    }
  memset((void *)AVI,0,sizeof(avi_t));
  
  AVI->mode = AVI_MODE_READ; /* open for reading */
  
  // file alread open
  AVI->fdes = fd;
  
  avi_parse_input_file(AVI, getIndex);

  AVI->aptr=0; //reset
  
  return AVI;
}

int avi_parse_input_file(avi_t *AVI, int getIndex)
{
  long i, n, rate, scale, idx_type;
  unsigned char *hdrl_data;
  long header_offset=0, hdrl_len=0;
  long nvi, nai[AVI_MAX_TRACKS], ioff;
  long tot[AVI_MAX_TRACKS];
  int j;
  int lasttag = 0;
  int vids_strh_seen = 0;
  int vids_strf_seen = 0;
  int auds_strh_seen = 0;
  //  int auds_strf_seen = 0;
  int num_stream = 0;
  char data[256];
  
  /* Read first 12 bytes and check that this is an AVI file */

   if( avi_read(AVI->fdes,data,12) != 12 ) ERR_EXIT(AVI_ERR_READ)

   if( strncasecmp(data  ,"RIFF",4) !=0 ||
       strncasecmp(data+8,"AVI ",4) !=0 ) ERR_EXIT(AVI_ERR_NO_AVI)

   /* Go through the AVI file and extract the header list,
      the start position of the 'movi' list and an optionally
      present idx1 tag */

   hdrl_data = 0;

   while(1)
   {
      if( avi_read(AVI->fdes,data,8) != 8 ) break; /* We assume it's EOF */

      n = str2ulong((unsigned char *) data+4);
      n = PAD_EVEN(n);

      if(strncasecmp(data,"LIST",4) == 0)
      {
         if( avi_read(AVI->fdes,data,4) != 4 ) ERR_EXIT(AVI_ERR_READ)
         n -= 4;
         if(strncasecmp(data,"hdrl",4) == 0)
         {
            hdrl_len = n;
            hdrl_data = (unsigned char *) malloc(n);
            if(hdrl_data==0) ERR_EXIT(AVI_ERR_NO_MEM);
				 
	    // offset of header
	    
	    header_offset = lseek(AVI->fdes,0,SEEK_CUR);
				 
            if( avi_read(AVI->fdes,(char *)hdrl_data,n) != n ) ERR_EXIT(AVI_ERR_READ)
         }
         else if(strncasecmp(data,"movi",4) == 0)
         {
            AVI->movi_start = lseek(AVI->fdes,0,SEEK_CUR);
            lseek(AVI->fdes,n,SEEK_CUR);
         }
         else
            lseek(AVI->fdes,n,SEEK_CUR);
      }
      else if(strncasecmp(data,"idx1",4) == 0)
      {
         /* n must be a multiple of 16, but the reading does not
            break if this is not the case */

         AVI->n_idx = AVI->max_idx = n/16;
         AVI->idx = (unsigned  char((*)[16]) ) malloc(n);
         if(AVI->idx==0) ERR_EXIT(AVI_ERR_NO_MEM)
         if(avi_read(AVI->fdes, (char *) AVI->idx, n) != n ) ERR_EXIT(AVI_ERR_READ)
      }
      else
         lseek(AVI->fdes,n,SEEK_CUR);
   }

   if(!hdrl_data      ) ERR_EXIT(AVI_ERR_NO_HDRL)
   if(!AVI->movi_start) ERR_EXIT(AVI_ERR_NO_MOVI)

   /* Interpret the header list */

   for(i=0;i<hdrl_len;)
   {
      /* List tags are completly ignored */

      if(strncasecmp((char *) hdrl_data+i, "LIST",4)==0) { i+= 12; continue; }

      n = str2ulong(hdrl_data+i+4);
      n = PAD_EVEN(n);

      /* Interpret the tag and its args */

      if(strncasecmp((char *)hdrl_data+i,"strh",4)==0)
      {
         i += 8;
         if(strncasecmp((char *)hdrl_data+i,"vids",4) == 0 && !vids_strh_seen)
         {
            memcpy(AVI->compressor,hdrl_data+i+4,4);
            AVI->compressor[4] = 0;

	    // ThOe
	    AVI->v_codech_off = header_offset + i+4;

            scale = str2ulong((unsigned char *)hdrl_data+i+20);
            rate  = str2ulong(hdrl_data+i+24);
            if(scale!=0) AVI->fps = (double)rate/(double)scale;
            AVI->video_frames = str2ulong(hdrl_data+i+32);
            AVI->video_strn = num_stream;
	    AVI->max_len = 0;
            vids_strh_seen = 1;
            lasttag = 1; /* vids */
         }
         else if (strncasecmp ((char *) hdrl_data+i,"auds",4) ==0 && ! auds_strh_seen)
         {

	   //inc audio tracks
	   AVI->aptr=AVI->anum;
	   ++AVI->anum;
	   
	   if(AVI->anum > AVI_MAX_TRACKS) {
	     fprintf(stderr, "error - only %d audio tracks supported\n", AVI_MAX_TRACKS);
	     return(-1);
	   }
	   
	   AVI->track[AVI->aptr].audio_bytes = str2ulong(hdrl_data+i+32)*avi_sampsize(AVI, 0);
	   AVI->track[AVI->aptr].audio_strn = num_stream;
	   //	   auds_strh_seen = 1;
	   lasttag = 2; /* auds */
	   
	   // ThOe
	   AVI->track[AVI->aptr].a_codech_off = header_offset + i;
	   
         }
         else
            lasttag = 0;
         num_stream++;
      }
      else if(strncasecmp((char *) hdrl_data+i,"strf",4)==0)
      {
         i += 8;
         if(lasttag == 1)
         {
            AVI->width  = str2ulong(hdrl_data+i+4);
            AVI->height = str2ulong(hdrl_data+i+8);
            vids_strf_seen = 1;
	    //ThOe
	    AVI->v_codecf_off = header_offset + i+16;

	    memcpy(AVI->compressor2, hdrl_data+i+16, 4);
            AVI->compressor2[4] = 0;

         }
         else if(lasttag == 2)
         {
            AVI->track[AVI->aptr].a_fmt   = str2ushort(hdrl_data+i  );

	    //ThOe
	    AVI->track[AVI->aptr].a_codecf_off = header_offset + i;
	    
            AVI->track[AVI->aptr].a_chans = str2ushort(hdrl_data+i+2);
            AVI->track[AVI->aptr].a_rate  = str2ulong (hdrl_data+i+4);
	    //ThOe: read mp3bitrate
	    AVI->track[AVI->aptr].mp3rate = 8*str2ulong(hdrl_data+i+8)/1000;
	    //:ThOe
            AVI->track[AVI->aptr].a_bits  = str2ushort(hdrl_data+i+14);
	    //            auds_strf_seen = 1;
         }
         lasttag = 0;
      }
      else
      {
         i += 8;
         lasttag = 0;
      }

      i += n;
   }

   free(hdrl_data);

   if(!vids_strh_seen || !vids_strf_seen) ERR_EXIT(AVI_ERR_NO_VIDS)

   AVI->video_tag[0] = AVI->video_strn/10 + '0';
   AVI->video_tag[1] = AVI->video_strn%10 + '0';
   AVI->video_tag[2] = 'd';
   AVI->video_tag[3] = 'b';

   /* Audio tag is set to "99wb" if no audio present */
   if(!AVI->track[0].a_chans) AVI->track[0].audio_strn = 99;

   for(j=0; j<AVI->anum; ++j) {
     AVI->track[j].audio_tag[0] = (j+1)/10 + '0';
     AVI->track[j].audio_tag[1] = (j+1)%10 + '0';
     AVI->track[j].audio_tag[2] = 'w';
     AVI->track[j].audio_tag[3] = 'b';
   }

   lseek(AVI->fdes,AVI->movi_start,SEEK_SET);

   /* get index if wanted */

   if(!getIndex) return(0);

   /* if the file has an idx1, check if this is relative
      to the start of the file or to the start of the movi list */

   idx_type = 0;

   if(AVI->idx)
   {
      long pos, len;

      /* Search the first videoframe in the idx1 and look where
         it is in the file */

      for(i=0;i<AVI->n_idx;i++)
         if( strncasecmp((char *) AVI->idx[i],(char *) AVI->video_tag,3)==0 ) break;
      if(i>=AVI->n_idx) ERR_EXIT(AVI_ERR_NO_VIDS)

      pos = str2ulong(AVI->idx[i]+ 8);
      len = str2ulong(AVI->idx[i]+12);

      lseek(AVI->fdes,pos,SEEK_SET);
      if(avi_read(AVI->fdes,data,8)!=8) ERR_EXIT(AVI_ERR_READ)
      if( strncasecmp((char *)data,(char *)AVI->idx[i],4)==0 && 
      str2ulong((unsigned char *)data+4)==len )
      {
         idx_type = 1; /* Index from start of file */
      }
      else
      {
         lseek(AVI->fdes,pos+AVI->movi_start-4,SEEK_SET);
         if(avi_read(AVI->fdes,data,8)!=8) ERR_EXIT(AVI_ERR_READ)
         if( strncasecmp((char *)data,(char *)AVI->idx[i],4)==0 && str2ulong((unsigned char *)data+4)==len )
         {
            idx_type = 2; /* Index from start of movi list */
         }
      }
      /* idx_type remains 0 if neither of the two tests above succeeds */
   }

   if(idx_type == 0)
   {
      /* we must search through the file to get the index */

      lseek(AVI->fdes, AVI->movi_start, SEEK_SET);

      AVI->n_idx = 0;

      while(1)
      {
         if( avi_read(AVI->fdes,data,8) != 8 ) break;
         n = str2ulong((unsigned char *)data+4);

         /* The movi list may contain sub-lists, ignore them */

         if(strncasecmp(data,"LIST",4)==0)
         {
            lseek(AVI->fdes,4,SEEK_CUR);
            continue;
         }

         /* Check if we got a tag ##db, ##dc or ##wb */
	 
         if( ( (data[2]=='d' || data[2]=='D') &&
               (data[3]=='b' || data[3]=='B' || data[3]=='c' || data[3]=='C') )
	     || ( (data[2]=='w' || data[2]=='W') &&
		  (data[3]=='b' || data[3]=='B') ) )
	   {
	   avi_add_index_entry(AVI,(unsigned char *) data,0,lseek(AVI->fdes,0,SEEK_CUR)-8,n);
         }
	 
         lseek(AVI->fdes,PAD_EVEN(n),SEEK_CUR);
      }
      idx_type = 1;
   }

   /* Now generate the video index and audio index arrays */

   nvi = 0;
   for(j=0; j<AVI->anum; ++j) nai[j] = 0;

   for(i=0;i<AVI->n_idx;i++) {
     
     if(strncasecmp((char *)AVI->idx[i],(char *) AVI->video_tag,3) == 0) nvi++;
     
     for(j=0; j<AVI->anum; ++j) if(strncasecmp((char *)AVI->idx[i], AVI->track[j].audio_tag,4) == 0) nai[j]++;
   }
   
   AVI->video_frames = nvi;
   for(j=0; j<AVI->anum; ++j) AVI->track[j].audio_chunks = nai[j];

//   fprintf(stderr, "chunks = %ld %d %s\n", AVI->track[0].audio_chunks, AVI->anum, AVI->track[0].audio_tag);

   if(AVI->video_frames==0) ERR_EXIT(AVI_ERR_NO_VIDS);
   AVI->video_index = (video_index_entry *) malloc(nvi*sizeof(video_index_entry));
   if(AVI->video_index==0) ERR_EXIT(AVI_ERR_NO_MEM);
   
   for(j=0; j<AVI->anum; ++j) {
       if(AVI->track[j].audio_chunks) {
	   AVI->track[j].audio_index = (audio_index_entry *) malloc(nai[j]*sizeof(audio_index_entry));
	   if(AVI->track[j].audio_index==0) ERR_EXIT(AVI_ERR_NO_MEM);
       }
   }   
   
   nvi = 0;
   for(j=0; j<AVI->anum; ++j) nai[j] = tot[j] = 0;
   
   ioff = idx_type == 1 ? 8 : AVI->movi_start+4;
   
   for(i=0;i<AVI->n_idx;i++) {

     //video
     if(strncasecmp((char *)AVI->idx[i],(char *)AVI->video_tag,3) == 0) {
       AVI->video_index[nvi].key = str2ulong(AVI->idx[i]+ 4);
       AVI->video_index[nvi].pos = str2ulong(AVI->idx[i]+ 8)+ioff;
       AVI->video_index[nvi].len = str2ulong(AVI->idx[i]+12);
       nvi++;
     }
     
     //audio
     for(j=0; j<AVI->anum; ++j) {
	 
       if(strncasecmp((char *)AVI->idx[i],AVI->track[j].audio_tag,4) == 0) {
	 AVI->track[j].audio_index[nai[j]].pos = str2ulong(AVI->idx[i]+ 8)+ioff;
	 AVI->track[j].audio_index[nai[j]].len = str2ulong(AVI->idx[i]+12);
	 AVI->track[j].audio_index[nai[j]].tot = tot[j];
	 tot[j] += AVI->track[j].audio_index[nai[j]].len;
	 nai[j]++;
       }
     }
   }
   
   
   for(j=0; j<AVI->anum; ++j) AVI->track[j].audio_bytes = tot[j];
   
   /* Reposition the file */
   
   lseek(AVI->fdes,AVI->movi_start,SEEK_SET);
   AVI->video_pos = 0;

   return(0);
}

long AVI_video_frames(avi_t *AVI)
{
   return AVI->video_frames;
}
int  AVI_video_width(avi_t *AVI)
{
   return AVI->width;
}
int  AVI_video_height(avi_t *AVI)
{
   return AVI->height;
}
double AVI_frame_rate(avi_t *AVI)
{
   return AVI->fps;
}
char* AVI_video_compressor(avi_t *AVI)
{
   return AVI->compressor2;
}

long AVI_max_video_chunk(avi_t *AVI)
{
   return AVI->max_len; 
}

int AVI_audio_tracks(avi_t *AVI)
{
    return(AVI->anum);
}

int AVI_audio_channels(avi_t *AVI)
{
   return AVI->track[AVI->aptr].a_chans;
}

long AVI_audio_mp3rate(avi_t *AVI)
{
   return AVI->track[AVI->aptr].mp3rate;
}

int AVI_audio_bits(avi_t *AVI)
{
   return AVI->track[AVI->aptr].a_bits;
}

int AVI_audio_format(avi_t *AVI)
{
   return AVI->track[AVI->aptr].a_fmt;
}

long AVI_audio_rate(avi_t *AVI)
{
   return AVI->track[AVI->aptr].a_rate;
}

long AVI_audio_bytes(avi_t *AVI)
{
   return AVI->track[AVI->aptr].audio_bytes;
}

long AVI_audio_chunks(avi_t *AVI)
{
   return AVI->track[AVI->aptr].audio_chunks;
}

long AVI_audio_codech_offset(avi_t *AVI)
{
   return AVI->track[AVI->aptr].a_codech_off;
}

long AVI_audio_codecf_offset(avi_t *AVI)
{
   return AVI->track[AVI->aptr].a_codecf_off;
}

long  AVI_video_codech_offset(avi_t *AVI)
{
    return AVI->v_codech_off;
}

long  AVI_video_codecf_offset(avi_t *AVI)
{
    return AVI->v_codecf_off;
}

long AVI_frame_size(avi_t *AVI, long frame)
{
   if(AVI->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
   if(!AVI->video_index)         { AVI_errno = AVI_ERR_NO_IDX;   return -1; }

   if(frame < 0 || frame >= AVI->video_frames) return 0;
   return(AVI->video_index[frame].len);
}

long AVI_audio_size(avi_t *AVI, long frame)
{
  if(AVI->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
  if(!AVI->track[AVI->aptr].audio_index)         { AVI_errno = AVI_ERR_NO_IDX;   return -1; }
  
  if(frame < 0 || frame >= AVI->track[AVI->aptr].audio_chunks) return 0;
  return(AVI->track[AVI->aptr].audio_index[frame].len);
}

long AVI_get_video_position(avi_t *AVI, long frame)
{
   if(AVI->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
   if(!AVI->video_index)         { AVI_errno = AVI_ERR_NO_IDX;   return -1; }

   if(frame < 0 || frame >= AVI->video_frames) return 0;
   return(AVI->video_index[frame].pos);
}


int AVI_seek_start(avi_t *AVI)
{
   if(AVI->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }

   lseek(AVI->fdes,AVI->movi_start,SEEK_SET);
   AVI->video_pos = 0;
   return 0;
}

int AVI_set_video_position(avi_t *AVI, long frame)
{
   if(AVI->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
   if(!AVI->video_index)         { AVI_errno = AVI_ERR_NO_IDX;   return -1; }

   if (frame < 0 ) frame = 0;
   AVI->video_pos = frame;
   return 0;
}

int AVI_set_audio_bitrate(avi_t *AVI, long bitrate)
{
   if(AVI->mode==AVI_MODE_READ) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }

   AVI->track[AVI->aptr].mp3rate = bitrate;
   return 0;
}
      

long AVI_read_frame(avi_t *AVI, char *vidbuf, int *keyframe)
{
   long n;

   if(AVI->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
   if(!AVI->video_index)         { AVI_errno = AVI_ERR_NO_IDX;   return -1; }

   if(AVI->video_pos < 0 || AVI->video_pos >= AVI->video_frames) return -1;
   n = AVI->video_index[AVI->video_pos].len;

   *keyframe = (AVI->video_index[AVI->video_pos].key==0x10) ? 1:0;

   lseek(AVI->fdes, AVI->video_index[AVI->video_pos].pos, SEEK_SET);

   if (avi_read(AVI->fdes,vidbuf,n) != n)
   {
      AVI_errno = AVI_ERR_READ;
      return -1;
   }

   AVI->video_pos++;

   return n;
}

int AVI_set_audio_position(avi_t *AVI, long byte)
{
   long n0, n1, n;

   if(AVI->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
   if(!AVI->track[AVI->aptr].audio_index)         { AVI_errno = AVI_ERR_NO_IDX;   return -1; }

   if(byte < 0) byte = 0;

   /* Binary search in the audio chunks */

   n0 = 0;
   n1 = AVI->track[AVI->aptr].audio_chunks;

   while(n0<n1-1)
   {
      n = (n0+n1)/2;
      if(AVI->track[AVI->aptr].audio_index[n].tot>byte)
         n1 = n;
      else
         n0 = n;
   }

   AVI->track[AVI->aptr].audio_posc = n0;
   AVI->track[AVI->aptr].audio_posb = byte - AVI->track[AVI->aptr].audio_index[n0].tot;

   return 0;
}

long AVI_read_audio(avi_t *AVI, char *audbuf, long bytes)
{
   long nr, pos, left, todo;

   if(AVI->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
   if(!AVI->track[AVI->aptr].audio_index)         { AVI_errno = AVI_ERR_NO_IDX;   return -1; }

   nr = 0; /* total number of bytes read */

   while(bytes>0)
   {
      left = AVI->track[AVI->aptr].audio_index[AVI->track[AVI->aptr].audio_posc].len - AVI->track[AVI->aptr].audio_posb;
      if(left==0)
      {
         if(AVI->track[AVI->aptr].audio_posc>=AVI->track[AVI->aptr].audio_chunks-1) return nr;
         AVI->track[AVI->aptr].audio_posc++;
         AVI->track[AVI->aptr].audio_posb = 0;
         continue;
      }
      if(bytes<left)
         todo = bytes;
      else
         todo = left;
      pos = AVI->track[AVI->aptr].audio_index[AVI->track[AVI->aptr].audio_posc].pos + AVI->track[AVI->aptr].audio_posb;
      lseek(AVI->fdes, pos, SEEK_SET);
      if (avi_read(AVI->fdes,audbuf+nr,todo) != todo)
      {
         AVI_errno = AVI_ERR_READ;
         return -1;
      }
      bytes -= todo;
      nr    += todo;
      AVI->track[AVI->aptr].audio_posb += todo;
   }

   return nr;
}

/* AVI_read_data: Special routine for reading the next audio or video chunk
                  without having an index of the file. */

int AVI_read_data(avi_t *AVI, char *vidbuf, long max_vidbuf,
                              char *audbuf, long max_audbuf,
                              long *len)
{

/*
 * Return codes:
 *
 *    1 = video data read
 *    2 = audio data read
 *    0 = reached EOF
 *   -1 = video buffer too small
 *   -2 = audio buffer too small
 */

   int n;
   char data[8];
 
   if(AVI->mode==AVI_MODE_WRITE) return 0;

   while(1)
   {
      /* Read tag and length */

      if( avi_read(AVI->fdes,data,8) != 8 ) return 0;

      /* if we got a list tag, ignore it */

      if(strncasecmp(data,"LIST",4) == 0)
      {
         lseek(AVI->fdes,4,SEEK_CUR);
         continue;
      }

      n = PAD_EVEN(str2ulong((unsigned char *)data+4));

      if(strncasecmp(data,AVI->video_tag,3) == 0)
      {
         *len = n;
         AVI->video_pos++;
         if(n>max_vidbuf)
         {
            lseek(AVI->fdes,n,SEEK_CUR);
            return -1;
         }
         if(avi_read(AVI->fdes,vidbuf,n) != n ) return 0;
         return 1;
      }
      else if(strncasecmp(data,AVI->track[AVI->aptr].audio_tag,4) == 0)
      {
         *len = n;
         if(n>max_audbuf)
         {
            lseek(AVI->fdes,n,SEEK_CUR);
            return -2;
         }
         if(avi_read(AVI->fdes,audbuf,n) != n ) return 0;
         return 2;
         break;
      }
      else
         if(lseek(AVI->fdes,n,SEEK_CUR)<0)  return 0;
   }
}

/* AVI_print_error: Print most recent error (similar to perror) */

char *(avi_errors[]) =
{
  /*  0 */ "avilib - No Error",
  /*  1 */ "avilib - AVI file size limit reached",
  /*  2 */ "avilib - Error opening AVI file",
  /*  3 */ "avilib - Error reading from AVI file",
  /*  4 */ "avilib - Error writing to AVI file",
  /*  5 */ "avilib - Error writing index (file may still be useable)",
  /*  6 */ "avilib - Error closing AVI file",
  /*  7 */ "avilib - Operation (read/write) not permitted",
  /*  8 */ "avilib - Out of memory (malloc failed)",
  /*  9 */ "avilib - Not an AVI file",
  /* 10 */ "avilib - AVI file has no header list (corrupted?)",
  /* 11 */ "avilib - AVI file has no MOVI list (corrupted?)",
  /* 12 */ "avilib - AVI file has no video data",
  /* 13 */ "avilib - operation needs an index",
  /* 14 */ "avilib - Unkown Error"
};
static int num_avi_errors = sizeof(avi_errors)/sizeof(char*);

static char error_string[4096];

void AVI_print_error(char *str)
{
   int aerrno;

   aerrno = (AVI_errno>=0 && AVI_errno<num_avi_errors) ? AVI_errno : num_avi_errors-1;

   fprintf(stderr,"%s: %s\n",str,avi_errors[aerrno]);

   /* for the following errors, perror should report a more detailed reason: */

   if(AVI_errno == AVI_ERR_OPEN ||
      AVI_errno == AVI_ERR_READ ||
      AVI_errno == AVI_ERR_WRITE ||
      AVI_errno == AVI_ERR_WRITE_INDEX ||
      AVI_errno == AVI_ERR_CLOSE )
   {
      perror("REASON");
   }
}

char *AVI_strerror()
{
   int aerrno;

   aerrno = (AVI_errno>=0 && AVI_errno<num_avi_errors) ? AVI_errno : num_avi_errors-1;

   if(AVI_errno == AVI_ERR_OPEN ||
      AVI_errno == AVI_ERR_READ ||
      AVI_errno == AVI_ERR_WRITE ||
      AVI_errno == AVI_ERR_WRITE_INDEX ||
      AVI_errno == AVI_ERR_CLOSE )
   {
      sprintf(error_string,"%s - %s",avi_errors[aerrno],strerror(errno));
      return error_string;
   }
   else
   {
      return avi_errors[aerrno];
   }
}

uint64_t AVI_max_size()
{
  return((uint64_t) AVI_MAX_LEN);
}

