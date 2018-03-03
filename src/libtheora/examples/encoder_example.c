/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2009                *
 * by the Xiph.Org Foundation and contributors http://www.xiph.org/ *
 *                                                                  *
 ********************************************************************

  function: example encoder application; makes an Ogg Theora/Vorbis
            file from YUV4MPEG2 and WAV input
  last mod: $Id: encoder_example.c 16517 2009-08-25 19:48:57Z giles $

 ********************************************************************/

#if !defined(_REENTRANT)
#define _REENTRANT
#endif
#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#if !defined(_LARGEFILE_SOURCE)
#define _LARGEFILE_SOURCE
#endif
#if !defined(_LARGEFILE64_SOURCE)
#define _LARGEFILE64_SOURCE
#endif
#if !defined(_FILE_OFFSET_BITS)
#define _FILE_OFFSET_BITS 64
#endif

#include <stdio.h>
#if !defined(_WIN32)
#include <getopt.h>
#include <unistd.h>
#else
#include "getopt.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "theora/theoraenc.h"
#include "vorbis/codec.h"
#include "vorbis/vorbisenc.h"

#ifdef _WIN32
/*supply missing headers and functions to Win32. going to hell, I know*/
#include <fcntl.h>
#include <io.h>

static double rint(double x)
{
  if (x < 0.0)
    return (double)(int)(x - 0.5);
  else
    return (double)(int)(x + 0.5);
}
#endif

const char *optstring = "b:e:o:a:A:v:V:s:S:f:F:ck:d:z:\1\2\3\4";
struct option options [] = {
  {"begin-time",required_argument,NULL,'b'},
  {"end-time",required_argument,NULL,'e'},
  {"output",required_argument,NULL,'o'},
  {"audio-rate-target",required_argument,NULL,'A'},
  {"video-rate-target",required_argument,NULL,'V'},
  {"audio-quality",required_argument,NULL,'a'},
  {"video-quality",required_argument,NULL,'v'},
  {"aspect-numerator",required_argument,NULL,'s'},
  {"aspect-denominator",required_argument,NULL,'S'},
  {"framerate-numerator",required_argument,NULL,'f'},
  {"framerate-denominator",required_argument,NULL,'F'},
  {"vp3-compatible",no_argument,NULL,'c'},
  {"speed",required_argument,NULL,'z'},
  {"soft-target",no_argument,NULL,'\1'},
  {"keyframe-freq",required_argument,NULL,'k'},
  {"buf-delay",required_argument,NULL,'d'},
  {"two-pass",no_argument,NULL,'\2'},
  {"first-pass",required_argument,NULL,'\3'},
  {"second-pass",required_argument,NULL,'\4'},
  {NULL,0,NULL,0}
};

/* You'll go to Hell for using globals. */

FILE *audio=NULL;
FILE *video=NULL;

int audio_ch=0;
int audio_hz=0;

float audio_q=.1f;
int audio_r=-1;
int vp3_compatible=0;

int frame_w=0;
int frame_h=0;
int pic_w=0;
int pic_h=0;
int pic_x=0;
int pic_y=0;
int video_fps_n=-1;
int video_fps_d=-1;
int video_par_n=-1;
int video_par_d=-1;
char interlace;
int src_c_dec_h=2;
int src_c_dec_v=2;
int dst_c_dec_h=2;
int dst_c_dec_v=2;
char chroma_type[16];

/*The size of each converted frame buffer.*/
size_t y4m_dst_buf_sz;
/*The amount to read directly into the converted frame buffer.*/
size_t y4m_dst_buf_read_sz;
/*The size of the auxilliary buffer.*/
size_t y4m_aux_buf_sz;
/*The amount to read into the auxilliary buffer.*/
size_t y4m_aux_buf_read_sz;

/*The function used to perform chroma conversion.*/
typedef void (*y4m_convert_func)(unsigned char *_dst,unsigned char *_aux);

y4m_convert_func y4m_convert=NULL;

int video_r=-1;
int video_q=-1;
ogg_uint32_t keyframe_frequency=0;
int buf_delay=-1;

long begin_sec=-1;
long begin_usec=0;
long end_sec=-1;
long end_usec=0;

static void usage(void){
  fprintf(stderr,
          "Usage: encoder_example [options] [audio_file] video_file\n\n"
          "Options: \n\n"
          "  -o --output <filename.ogv>      file name for encoded output;\n"
          "                                  If this option is not given, the\n"
          "                                  compressed data is sent to stdout.\n\n"
          "  -A --audio-rate-target <n>      bitrate target for Vorbis audio;\n"
          "                                  use -a and not -A if at all possible,\n"
          "                                  as -a gives higher quality for a given\n"
          "                                  bitrate.\n\n"
          "  -V --video-rate-target <n>      bitrate target for Theora video\n\n"
          "     --soft-target                Use a large reservoir and treat the rate\n"
          "                                  as a soft target; rate control is less\n"
          "                                  strict but resulting quality is usually\n"
          "                                  higher/smoother overall. Soft target also\n"
          "                                  allows an optional -v setting to specify\n"
          "                                  a minimum allowed quality.\n\n"
          "     --two-pass                   Compress input using two-pass rate control\n"
          "                                  This option requires that the input to the\n"
          "                                  to the encoder is seekable and performs\n"
          "                                  both passes automatically.\n\n"
          "     --first-pass <filename>      Perform first-pass of a two-pass rate\n"
          "                                  controlled encoding, saving pass data to\n"
          "                                  <filename> for a later second pass\n\n"
          "     --second-pass <filename>     Perform second-pass of a two-pass rate\n"
          "                                  controlled encoding, reading first-pass\n"
          "                                  data from <filename>.  The first pass\n"
          "                                  data must come from a first encoding pass\n"
          "                                  using identical input video to work\n"
          "                                  properly.\n\n"
          "  -a --audio-quality <n>          Vorbis quality selector from -1 to 10\n"
          "                                  (-1 yields smallest files but lowest\n"
          "                                  fidelity; 10 yields highest fidelity\n"
          "                                  but large files. '2' is a reasonable\n"
          "                                  default).\n\n"
          "   -v --video-quality <n>         Theora quality selector from 0 to 10\n"
          "                                  (0 yields smallest files but lowest\n"
          "                                  video quality. 10 yields highest\n"
          "                                  fidelity but large files).\n\n"
          "   -s --aspect-numerator <n>      Aspect ratio numerator, default is 0\n"
          "                                  or extracted from YUV input file\n"
          "   -S --aspect-denominator <n>    Aspect ratio denominator, default is 0\n"
          "                                  or extracted from YUV input file\n"
          "   -f --framerate-numerator <n>   Frame rate numerator, can be extracted\n"
          "                                  from YUV input file. ex: 30000000\n"
          "   -F --framerate-denominator <n> Frame rate denominator, can be extracted\n"
          "                                  from YUV input file. ex: 1000000\n"
          "                                  The frame rate nominator divided by this\n"
          "                                  determinates the frame rate in units per tick\n"
          "   -k --keyframe-freq <n>         Keyframe frequency\n"
          "   -z --speed <n>                 Sets the encoder speed level. Higher speed\n"
          "                                  levels favor quicker encoding over better\n"
          "                                  quality per bit. Depending on the encoding\n"
          "                                  mode, and the internal algorithms used,\n"
          "                                  quality may actually improve with higher\n"
          "                                  speeds, but in this case bitrate will also\n"
          "                                  likely increase. The maximum value, and the\n"
          "                                  meaning of each value, are implementation-\n"
          "                                  specific and may change depending on the\n"
          "                                  current encoding mode (rate constrained,\n"
          "                                  two-pass, etc.).\n"
          "   -d --buf-delay <n>             Buffer delay (in frames). Longer delays\n"
          "                                  allow smoother rate adaptation and provide\n"
          "                                  better overall quality, but require more\n"
          "                                  client side buffering and add latency. The\n"
          "                                  default value is the keyframe interval for\n"
          "                                  one-pass encoding (or somewhat larger if\n"
          "                                  --soft-target is used) and infinite for\n"
          "                                  two-pass encoding.\n"
          "   -b --begin-time <h:m:s.d>      Begin encoding at offset into input\n"
          "   -e --end-time <h:m:s.d>        End encoding at offset into input\n"
          "encoder_example accepts only uncompressed RIFF WAV format audio and\n"
          "YUV4MPEG2 uncompressed video.\n\n");
  exit(1);
}

static int y4m_parse_tags(char *_tags){
  int   got_w;
  int   got_h;
  int   got_fps;
  int   got_interlace;
  int   got_par;
  int   got_chroma;
  int   tmp_video_fps_n;
  int   tmp_video_fps_d;
  int   tmp_video_par_n;
  int   tmp_video_par_d;
  char *p;
  char *q;
  got_w=got_h=got_fps=got_interlace=got_par=got_chroma=0;
  for(p=_tags;;p=q){
    /*Skip any leading spaces.*/
    while(*p==' ')p++;
    /*If that's all we have, stop.*/
    if(p[0]=='\0')break;
    /*Find the end of this tag.*/
    for(q=p+1;*q!='\0'&&*q!=' ';q++);
    /*Process the tag.*/
    switch(p[0]){
      case 'W':{
        if(sscanf(p+1,"%d",&pic_w)!=1)return -1;
        got_w=1;
      }break;
      case 'H':{
        if(sscanf(p+1,"%d",&pic_h)!=1)return -1;
        got_h=1;
      }break;
      case 'F':{
        if(sscanf(p+1,"%d:%d",&tmp_video_fps_n,&tmp_video_fps_d)!=2)return -1;
        got_fps=1;
      }break;
      case 'I':{
        interlace=p[1];
        got_interlace=1;
      }break;
      case 'A':{
        if(sscanf(p+1,"%d:%d",&tmp_video_par_n,&tmp_video_par_d)!=2)return -1;
        got_par=1;
      }break;
      case 'C':{
        if(q-p>16)return -1;
        memcpy(chroma_type,p+1,q-p-1);
        chroma_type[q-p-1]='\0';
        got_chroma=1;
      }break;
      /*Ignore unknown tags.*/
    }
  }
  if(!got_w||!got_h||!got_fps||!got_interlace||!got_par)return -1;
  /*Chroma-type is not specified in older files, e.g., those generated by
     mplayer.*/
  if(!got_chroma)strcpy(chroma_type,"420");
  /*Update fps and aspect ratio globals if not specified in the command line.*/
  if(video_fps_n==-1)video_fps_n=tmp_video_fps_n;
  if(video_fps_d==-1)video_fps_d=tmp_video_fps_d;
  if(video_par_n==-1)video_par_n=tmp_video_par_n;
  if(video_par_d==-1)video_par_d=tmp_video_par_d;
  return 0;
}

/*All anti-aliasing filters in the following conversion functions are based on
   one of two window functions:
  The 6-tap Lanczos window (for down-sampling and shifts):
   sinc(\pi*t)*sinc(\pi*t/3), |t|<3  (sinc(t)==sin(t)/t)
   0,                         |t|>=3
  The 4-tap Mitchell window (for up-sampling):
   7|t|^3-12|t|^2+16/3,             |t|<1
   -(7/3)|x|^3+12|x|^2-20|x|+32/3,  |t|<2
   0,                               |t|>=2
  The number of taps is intentionally kept small to reduce computational
   overhead and limit ringing.

  The taps from these filters are scaled so that their sum is 1, and the result
   is scaled by 128 and rounded to integers to create a filter whose
   intermediate values fit inside 16 bits.
  Coefficients are rounded in such a way as to ensure their sum is still 128,
   which is usually equivalent to normal rounding.*/

#define OC_MINI(_a,_b)      ((_a)>(_b)?(_b):(_a))
#define OC_MAXI(_a,_b)      ((_a)<(_b)?(_b):(_a))
#define OC_CLAMPI(_a,_b,_c) (OC_MAXI(_a,OC_MINI(_b,_c)))

/*420jpeg chroma samples are sited like:
  Y-------Y-------Y-------Y-------
  |       |       |       |
  |   BR  |       |   BR  |
  |       |       |       |
  Y-------Y-------Y-------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |
  Y-------Y-------Y-------Y-------
  |       |       |       |
  |   BR  |       |   BR  |
  |       |       |       |
  Y-------Y-------Y-------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |

  420mpeg2 chroma samples are sited like:
  Y-------Y-------Y-------Y-------
  |       |       |       |
  BR      |       BR      |
  |       |       |       |
  Y-------Y-------Y-------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |
  Y-------Y-------Y-------Y-------
  |       |       |       |
  BR      |       BR      |
  |       |       |       |
  Y-------Y-------Y-------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |

  We use a resampling filter to shift the site locations one quarter pixel (at
   the chroma plane's resolution) to the right.
  The 4:2:2 modes look exactly the same, except there are twice as many chroma
   lines, and they are vertically co-sited with the luma samples in both the
   mpeg2 and jpeg cases (thus requiring no vertical resampling).*/
static void y4m_convert_42xmpeg2_42xjpeg(unsigned char *_dst,
 unsigned char *_aux){
  int c_w;
  int c_h;
  int pli;
  int y;
  int x;
  /*Skip past the luma data.*/
  _dst+=pic_w*pic_h;
  /*Compute the size of each chroma plane.*/
  c_w=(pic_w+dst_c_dec_h-1)/dst_c_dec_h;
  c_h=(pic_h+dst_c_dec_v-1)/dst_c_dec_v;
  for(pli=1;pli<3;pli++){
    for(y=0;y<c_h;y++){
      /*Filter: [4 -17 114 35 -9 1]/128, derived from a 6-tap Lanczos
         window.*/
      for(x=0;x<OC_MINI(c_w,2);x++){
        _dst[x]=(unsigned char)OC_CLAMPI(0,4*_aux[0]-17*_aux[OC_MAXI(x-1,0)]+
         114*_aux[x]+35*_aux[OC_MINI(x+1,c_w-1)]-9*_aux[OC_MINI(x+2,c_w-1)]+
         _aux[OC_MINI(x+3,c_w-1)]+64>>7,255);
      }
      for(;x<c_w-3;x++){
        _dst[x]=(unsigned char)OC_CLAMPI(0,4*_aux[x-2]-17*_aux[x-1]+
         114*_aux[x]+35*_aux[x+1]-9*_aux[x+2]+_aux[x+3]+64>>7,255);
      }
      for(;x<c_w;x++){
        _dst[x]=(unsigned char)OC_CLAMPI(0,4*_aux[x-2]-17*_aux[x-1]+
         114*_aux[x]+35*_aux[OC_MINI(x+1,c_w-1)]-9*_aux[OC_MINI(x+2,c_w-1)]+
         _aux[c_w-1]+64>>7,255);
      }
      _dst+=c_w;
      _aux+=c_w;
    }
  }
}

/*This format is only used for interlaced content, but is included for
   completeness.

  420jpeg chroma samples are sited like:
  Y-------Y-------Y-------Y-------
  |       |       |       |
  |   BR  |       |   BR  |
  |       |       |       |
  Y-------Y-------Y-------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |
  Y-------Y-------Y-------Y-------
  |       |       |       |
  |   BR  |       |   BR  |
  |       |       |       |
  Y-------Y-------Y-------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |

  420paldv chroma samples are sited like:
  YR------Y-------YR------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |
  YB------Y-------YB------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |
  YR------Y-------YR------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |
  YB------Y-------YB------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |

  We use a resampling filter to shift the site locations one quarter pixel (at
   the chroma plane's resolution) to the right.
  Then we use another filter to move the C_r location down one quarter pixel,
   and the C_b location up one quarter pixel.*/
static void y4m_convert_42xpaldv_42xjpeg(unsigned char *_dst,
 unsigned char *_aux){
  unsigned char *tmp;
  int            c_w;
  int            c_h;
  int            c_sz;
  int            pli;
  int            y;
  int            x;
  /*Skip past the luma data.*/
  _dst+=pic_w*pic_h;
  /*Compute the size of each chroma plane.*/
  c_w=(pic_w+1)/2;
  c_h=(pic_h+dst_c_dec_h-1)/dst_c_dec_h;
  c_sz=c_w*c_h;
  /*First do the horizontal re-sampling.
    This is the same as the mpeg2 case, except that after the horizontal case,
     we need to apply a second vertical filter.*/
  tmp=_aux+2*c_sz;
  for(pli=1;pli<3;pli++){
    for(y=0;y<c_h;y++){
      /*Filter: [4 -17 114 35 -9 1]/128, derived from a 6-tap Lanczos
         window.*/
      for(x=0;x<OC_MINI(c_w,2);x++){
        tmp[x]=(unsigned char)OC_CLAMPI(0,4*_aux[0]-17*_aux[OC_MAXI(x-1,0)]+
         114*_aux[x]+35*_aux[OC_MINI(x+1,c_w-1)]-9*_aux[OC_MINI(x+2,c_w-1)]+
         _aux[OC_MINI(x+3,c_w-1)]+64>>7,255);
      }
      for(;x<c_w-3;x++){
        tmp[x]=(unsigned char)OC_CLAMPI(0,4*_aux[x-2]-17*_aux[x-1]+
         114*_aux[x]+35*_aux[x+1]-9*_aux[x+2]+_aux[x+3]+64>>7,255);
      }
      for(;x<c_w;x++){
        tmp[x]=(unsigned char)OC_CLAMPI(0,4*_aux[x-2]-17*_aux[x-1]+
         114*_aux[x]+35*_aux[OC_MINI(x+1,c_w-1)]-9*_aux[OC_MINI(x+2,c_w-1)]+
         _aux[c_w-1]+64>>7,255);
      }
      tmp+=c_w;
      _aux+=c_w;
    }
    switch(pli){
      case 1:{
        tmp-=c_sz;
        /*Slide C_b up a quarter-pel.
          This is the same filter used above, but in the other order.*/
        for(x=0;x<c_w;x++){
          for(y=0;y<OC_MINI(c_h,3);y++){
            _dst[y*c_w]=(unsigned char)OC_CLAMPI(0,tmp[0]-
             9*tmp[OC_MAXI(y-2,0)*c_w]+35*tmp[OC_MAXI(y-1,0)*c_w]+
             114*tmp[y*c_w]-17*tmp[OC_MINI(y+1,c_h-1)*c_w]+
             4*tmp[OC_MINI(y+2,c_h-1)*c_w]+64>>7,255);
          }
          for(;y<c_h-2;y++){
            _dst[y*c_w]=(unsigned char)OC_CLAMPI(0,tmp[(y-3)*c_w]-
             9*tmp[(y-2)*c_w]+35*tmp[(y-1)*c_w]+114*tmp[y*c_w]-
             17*tmp[(y+1)*c_w]+4*tmp[(y+2)*c_w]+64>>7,255);
          }
          for(;y<c_h;y++){
            _dst[y*c_w]=(unsigned char)OC_CLAMPI(0,tmp[(y-3)*c_w]-
             9*tmp[(y-2)*c_w]+35*tmp[(y-1)*c_w]+114*tmp[y*c_w]-
             17*tmp[OC_MINI(y+1,c_h-1)*c_w]+4*tmp[(c_h-1)*c_w]+64>>7,255);
          }
          _dst++;
          tmp++;
        }
        _dst+=c_sz-c_w;
        tmp-=c_w;
      }break;
      case 2:{
        tmp-=c_sz;
        /*Slide C_r down a quarter-pel.
          This is the same as the horizontal filter.*/
        for(x=0;x<c_w;x++){
          for(y=0;y<OC_MINI(c_h,2);y++){
            _dst[y*c_w]=(unsigned char)OC_CLAMPI(0,4*tmp[0]-
             17*tmp[OC_MAXI(y-1,0)*c_w]+114*tmp[y*c_w]+
             35*tmp[OC_MINI(y+1,c_h-1)*c_w]-9*tmp[OC_MINI(y+2,c_h-1)*c_w]+
             tmp[OC_MINI(y+3,c_h-1)*c_w]+64>>7,255);
          }
          for(;y<c_h-3;y++){
            _dst[y*c_w]=(unsigned char)OC_CLAMPI(0,4*tmp[(y-2)*c_w]-
             17*tmp[(y-1)*c_w]+114*tmp[y*c_w]+35*tmp[(y+1)*c_w]-
             9*tmp[(y+2)*c_w]+tmp[(y+3)*c_w]+64>>7,255);
          }
          for(;y<c_h;y++){
            _dst[y*c_w]=(unsigned char)OC_CLAMPI(0,4*tmp[(y-2)*c_w]-
             17*tmp[(y-1)*c_w]+114*tmp[y*c_w]+35*tmp[OC_MINI(y+1,c_h-1)*c_w]-
             9*tmp[OC_MINI(y+2,c_h-1)*c_w]+tmp[(c_h-1)*c_w]+64>>7,255);
          }
          _dst++;
          tmp++;
        }
      }break;
    }
    /*For actual interlaced material, this would have to be done separately on
       each field, and the shift amounts would be different.
      C_r moves down 1/8, C_b up 3/8 in the top field, and C_r moves down 3/8,
       C_b up 1/8 in the bottom field.
      The corresponding filters would be:
       Down 1/8 (reverse order for up): [3 -11 125 15 -4 0]/128
       Down 3/8 (reverse order for up): [4 -19 98 56 -13 2]/128*/
  }
}

/*422jpeg chroma samples are sited like:
  Y---BR--Y-------Y---BR--Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |
  Y---BR--Y-------Y---BR--Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |
  Y---BR--Y-------Y---BR--Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |
  Y---BR--Y-------Y---BR--Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |

  411 chroma samples are sited like:
  YBR-----Y-------Y-------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |
  YBR-----Y-------Y-------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |
  YBR-----Y-------Y-------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |
  YBR-----Y-------Y-------Y-------
  |       |       |       |
  |       |       |       |
  |       |       |       |

  We use a filter to resample at site locations one eighth pixel (at the source
   chroma plane's horizontal resolution) and five eighths of a pixel to the
   right.*/
static void y4m_convert_411_422jpeg(unsigned char *_dst,
 unsigned char *_aux){
  int c_w;
  int dst_c_w;
  int c_h;
  int pli;
  int y;
  int x;
  /*Skip past the luma data.*/
  _dst+=pic_w*pic_h;
  /*Compute the size of each chroma plane.*/
  c_w=(pic_w+src_c_dec_h-1)/src_c_dec_h;
  dst_c_w=(pic_w+dst_c_dec_h-1)/dst_c_dec_h;
  c_h=(pic_h+dst_c_dec_v-1)/dst_c_dec_v;
  for(pli=1;pli<3;pli++){
    for(y=0;y<c_h;y++){
      /*Filters: [1 110 18 -1]/128 and [-3 50 86 -5]/128, both derived from a
         4-tap Mitchell window.*/
      for(x=0;x<OC_MINI(c_w,1);x++){
        _dst[x<<1]=(unsigned char)OC_CLAMPI(0,111*_aux[0]+
         18*_aux[OC_MINI(1,c_w-1)]-_aux[OC_MINI(2,c_w-1)]+64>>7,255);
        _dst[x<<1|1]=(unsigned char)OC_CLAMPI(0,47*_aux[0]+
         86*_aux[OC_MINI(1,c_w-1)]-5*_aux[OC_MINI(2,c_w-1)]+64>>7,255);
      }
      for(;x<c_w-2;x++){
        _dst[x<<1]=(unsigned char)OC_CLAMPI(0,_aux[x-1]+110*_aux[x]+
         18*_aux[x+1]-_aux[x+2]+64>>7,255);
        _dst[x<<1|1]=(unsigned char)OC_CLAMPI(0,-3*_aux[x-1]+50*_aux[x]+
         86*_aux[x+1]-5*_aux[x+2]+64>>7,255);
      }
      for(;x<c_w;x++){
        _dst[x<<1]=(unsigned char)OC_CLAMPI(0,_aux[x-1]+110*_aux[x]+
         18*_aux[OC_MINI(x+1,c_w-1)]-_aux[c_w-1]+64>>7,255);
        if((x<<1|1)<dst_c_w){
          _dst[x<<1|1]=(unsigned char)OC_CLAMPI(0,-3*_aux[x-1]+50*_aux[x]+
           86*_aux[OC_MINI(x+1,c_w-1)]-5*_aux[c_w-1]+64>>7,255);
        }
      }
      _dst+=dst_c_w;
      _aux+=c_w;
    }
  }
}

/*The image is padded with empty chroma components at 4:2:0.
  This costs about 17 bits a frame to code.*/
static void y4m_convert_mono_420jpeg(unsigned char *_dst,
 unsigned char *_aux){
  int c_sz;
  _dst+=pic_w*pic_h;
  c_sz=((pic_w+dst_c_dec_h-1)/dst_c_dec_h)*((pic_h+dst_c_dec_v-1)/dst_c_dec_v);
  memset(_dst,128,c_sz*2);
}

#if 0
/*Right now just 444 to 420.
  Not too hard to generalize.*/
static void y4m_convert_4xxjpeg_42xjpeg(unsigned char *_dst,
 unsigned char *_aux){
  unsigned char *tmp;
  int            c_w;
  int            c_h;
  int            pic_sz;
  int            tmp_sz;
  int            c_sz;
  int            pli;
  int            y;
  int            x;
  /*Compute the size of each chroma plane.*/
  c_w=(pic_w+dst_c_dec_h-1)/dst_c_dec_h;
  c_h=(pic_h+dst_c_dec_v-1)/dst_c_dec_v;
  pic_sz=pic_w*pic_h;
  tmp_sz=c_w*pic_h;
  c_sz=c_w*c_h;
  _dst+=pic_sz;
  for(pli=1;pli<3;pli++){
    tmp=_aux+pic_sz;
    /*In reality, the horizontal and vertical steps could be pipelined, for
       less memory consumption and better cache performance, but we do them
       separately for simplicity.*/
    /*First do horizontal filtering (convert to 4:2:2)*/
    /*Filter: [3 -17 78 78 -17 3]/128, derived from a 6-tap Lanczos window.*/
    for(y=0;y<pic_h;y++){
      for(x=0;x<OC_MINI(pic_w,2);x+=2){
        tmp[x>>1]=OC_CLAMPI(0,64*_aux[0]+78*_aux[OC_MINI(1,pic_w-1)]-
         17*_aux[OC_MINI(2,pic_w-1)]+3*_aux[OC_MINI(3,pic_w-1)]+64>>7,255);
      }
      for(;x<pic_w-3;x+=2){
        tmp[x>>1]=OC_CLAMPI(0,3*(_aux[x-2]+_aux[x+3])-17*(_aux[x-1]+_aux[x+2])+
         78*(_aux[x]+_aux[x+1])+64>>7,255);
      }
      for(;x<pic_w;x+=2){
        tmp[x>>1]=OC_CLAMPI(0,3*(_aux[x-2]+_aux[pic_w-1])-
         17*(_aux[x-1]+_aux[OC_MINI(x+2,pic_w-1)])+
         78*(_aux[x]+_aux[OC_MINI(x+1,pic_w-1)])+64>>7,255);
      }
      tmp+=c_w;
      _aux+=pic_w;
    }
    _aux-=pic_sz;
    tmp-=tmp_sz;
    /*Now do the vertical filtering.*/
    for(x=0;x<c_w;x++){
      for(y=0;y<OC_MINI(pic_h,2);y+=2){
        _dst[(y>>1)*c_w]=OC_CLAMPI(0,64*tmp[0]+78*tmp[OC_MINI(1,pic_h-1)*c_w]-
         17*tmp[OC_MINI(2,pic_h-1)*c_w]+3*tmp[OC_MINI(3,pic_h-1)*c_w]+
         64>>7,255);
      }
      for(;y<pic_h-3;y+=2){
        _dst[(y>>1)*c_w]=OC_CLAMPI(0,3*(tmp[(y-2)*c_w]+tmp[(y+3)*c_w])-
         17*(tmp[(y-1)*c_w]+tmp[(y+2)*c_w])+78*(tmp[y*c_w]+tmp[(y+1)*c_w])+
         64>>7,255);
      }
      for(;y<pic_h;y+=2){
        _dst[(y>>1)*c_w]=OC_CLAMPI(0,3*(tmp[(y-2)*c_w]+tmp[(pic_h-1)*c_w])-
         17*(tmp[(y-1)*c_w]+tmp[OC_MINI(y+2,pic_h-1)*c_w])+
         78*(tmp[y*c_w]+tmp[OC_MINI(y+1,pic_h-1)*c_w])+64>>7,255);
      }
      tmp++;
      _dst++;
    }
    _dst-=c_w;
  }
}
#endif


/*No conversion function needed.*/
static void y4m_convert_null(unsigned char *_dst,
 unsigned char *_aux){
}

static void id_file(char *f){
  FILE *test;
  unsigned char buffer[80];
  int ret;

  /* open it, look for magic */

  if(!strcmp(f,"-")){
    /* stdin */
    test=stdin;
  }else{
    test=fopen(f,"rb");
    if(!test){
      fprintf(stderr,"Unable to open file %s.\n",f);
      exit(1);
    }
  }

  ret=fread(buffer,1,4,test);
  if(ret<4){
    fprintf(stderr,"EOF determining file type of file %s.\n",f);
    exit(1);
  }

  if(!memcmp(buffer,"RIFF",4)){
    /* possible WAV file */

    if(audio){
      /* umm, we already have one */
      fprintf(stderr,"Multiple RIFF WAVE files specified on command line.\n");
      exit(1);
    }

    /* Parse the rest of the header */

    ret=fread(buffer,1,8,test);
    if(ret<8)goto riff_err;
    if(!memcmp(buffer+4,"WAVE",4)){

      while(!feof(test)){
        ret=fread(buffer,1,4,test);
        if(ret<4)goto riff_err;
        if(!memcmp("fmt",buffer,3)){

          /* OK, this is our audio specs chunk.  Slurp it up. */

          ret=fread(buffer,1,20,test);
          if(ret<20)goto riff_err;

          if(memcmp(buffer+4,"\001\000",2)){
            fprintf(stderr,"The WAV file %s is in a compressed format; "
                    "can't read it.\n",f);
            exit(1);
          }

          audio=test;
          audio_ch=buffer[6]+(buffer[7]<<8);
          audio_hz=buffer[8]+(buffer[9]<<8)+
            (buffer[10]<<16)+(buffer[11]<<24);

          if(buffer[18]+(buffer[19]<<8)!=16){
            fprintf(stderr,"Can only read 16 bit WAV files for now.\n");
            exit(1);
          }

          /* Now, align things to the beginning of the data */
          /* Look for 'dataxxxx' */
          while(!feof(test)){
            ret=fread(buffer,1,4,test);
            if(ret<4)goto riff_err;
            if(!memcmp("data",buffer,4)){
              /* We're there.  Ignore the declared size for now. */
              ret=fread(buffer,1,4,test);
              if(ret<4)goto riff_err;

              fprintf(stderr,"File %s is 16 bit %d channel %d Hz RIFF WAV audio.\n",
                      f,audio_ch,audio_hz);

              return;
            }
          }
        }
      }
    }

    fprintf(stderr,"Couldn't find WAVE data in RIFF file %s.\n",f);
    exit(1);

  }
  if(!memcmp(buffer,"YUV4",4)){
    /* possible YUV2MPEG2 format file */
    /* read until newline, or 80 cols, whichever happens first */
    int i;
    for(i=0;i<79;i++){
      ret=fread(buffer+i,1,1,test);
      if(ret<1)goto yuv_err;
      if(buffer[i]=='\n')break;
    }
    if(i==79){
      fprintf(stderr,"Error parsing %s header; not a YUV2MPEG2 file?\n",f);
    }
    buffer[i]='\0';

    if(!memcmp(buffer,"MPEG",4)){

      if(video){
        /* umm, we already have one */
        fprintf(stderr,"Multiple video files specified on command line.\n");
        exit(1);
      }

      if(buffer[4]!='2'){
        fprintf(stderr,"Incorrect YUV input file version; YUV4MPEG2 required.\n");
      }

      ret=y4m_parse_tags((char *)buffer+5);
      if(ret<0){
        fprintf(stderr,"Error parsing YUV4MPEG2 header in file %s.\n",f);
        exit(1);
      }

      if(interlace!='p'){
        fprintf(stderr,"Input video is interlaced; Theora handles only progressive scan\n");
        exit(1);
      }

      if(strcmp(chroma_type,"420")==0||strcmp(chroma_type,"420jpeg")==0){
        src_c_dec_h=dst_c_dec_h=src_c_dec_v=dst_c_dec_v=2;
        y4m_dst_buf_read_sz=pic_w*pic_h+2*((pic_w+1)/2)*((pic_h+1)/2);
        y4m_aux_buf_sz=y4m_aux_buf_read_sz=0;
        y4m_convert=y4m_convert_null;
      }
      else if(strcmp(chroma_type,"420mpeg2")==0){
        src_c_dec_h=dst_c_dec_h=src_c_dec_v=dst_c_dec_v=2;
        y4m_dst_buf_read_sz=pic_w*pic_h;
        /*Chroma filter required: read into the aux buf first.*/
        y4m_aux_buf_sz=y4m_aux_buf_read_sz=2*((pic_w+1)/2)*((pic_h+1)/2);
        y4m_convert=y4m_convert_42xmpeg2_42xjpeg;
      }
      else if(strcmp(chroma_type,"420paldv")==0){
        src_c_dec_h=dst_c_dec_h=src_c_dec_v=dst_c_dec_v=2;
        y4m_dst_buf_read_sz=pic_w*pic_h;
        /*Chroma filter required: read into the aux buf first.
          We need to make two filter passes, so we need some extra space in the
           aux buffer.*/
        y4m_aux_buf_sz=3*((pic_w+1)/2)*((pic_h+1)/2);
        y4m_aux_buf_read_sz=2*((pic_w+1)/2)*((pic_h+1)/2);
        y4m_convert=y4m_convert_42xpaldv_42xjpeg;
      }
      else if(strcmp(chroma_type,"422")==0){
        src_c_dec_h=dst_c_dec_h=2;
        src_c_dec_v=dst_c_dec_v=1;
        y4m_dst_buf_read_sz=pic_w*pic_h;
        /*Chroma filter required: read into the aux buf first.*/
        y4m_aux_buf_sz=y4m_aux_buf_read_sz=2*((pic_w+1)/2)*pic_h;
        y4m_convert=y4m_convert_42xmpeg2_42xjpeg;
      }
      else if(strcmp(chroma_type,"411")==0){
        src_c_dec_h=4;
        /*We don't want to introduce any additional sub-sampling, so we
           promote 4:1:1 material to 4:2:2, as the closest format Theora can
           handle.*/
        dst_c_dec_h=2;
        src_c_dec_v=dst_c_dec_v=1;
        y4m_dst_buf_read_sz=pic_w*pic_h;
        /*Chroma filter required: read into the aux buf first.*/
        y4m_aux_buf_sz=y4m_aux_buf_read_sz=2*((pic_w+3)/4)*pic_h;
        y4m_convert=y4m_convert_411_422jpeg;
      }
      else if(strcmp(chroma_type,"444")==0){
        src_c_dec_h=dst_c_dec_h=src_c_dec_v=dst_c_dec_v=1;
        y4m_dst_buf_read_sz=pic_w*pic_h*3;
        y4m_aux_buf_sz=y4m_aux_buf_read_sz=0;
        y4m_convert=y4m_convert_null;
      }
      else if(strcmp(chroma_type,"444alpha")==0){
        src_c_dec_h=dst_c_dec_h=src_c_dec_v=dst_c_dec_v=1;
        y4m_dst_buf_read_sz=pic_w*pic_h*3;
        /*Read the extra alpha plane into the aux buf.
          It will be discarded.*/
        y4m_aux_buf_sz=y4m_aux_buf_read_sz=pic_w*pic_h;
        y4m_convert=y4m_convert_null;
      }
      else if(strcmp(chroma_type,"mono")==0){
        src_c_dec_h=src_c_dec_v=0;
        dst_c_dec_h=dst_c_dec_v=2;
        y4m_dst_buf_read_sz=pic_w*pic_h;
        y4m_aux_buf_sz=y4m_aux_buf_read_sz=0;
        y4m_convert=y4m_convert_mono_420jpeg;
      }
      else{
        fprintf(stderr,"Unknown chroma sampling type: %s\n",chroma_type);
        exit(1);
      }
      /*The size of the final frame buffers is always computed from the
         destination chroma decimation type.*/
      y4m_dst_buf_sz=pic_w*pic_h+2*((pic_w+dst_c_dec_h-1)/dst_c_dec_h)*
       ((pic_h+dst_c_dec_v-1)/dst_c_dec_v);

      video=test;

      fprintf(stderr,"File %s is %dx%d %.02f fps %s video.\n",
              f,pic_w,pic_h,(double)video_fps_n/video_fps_d,chroma_type);

      return;
    }
  }
  fprintf(stderr,"Input file %s is neither a WAV nor YUV4MPEG2 file.\n",f);
  exit(1);

 riff_err:
  fprintf(stderr,"EOF parsing RIFF file %s.\n",f);
  exit(1);
 yuv_err:
  fprintf(stderr,"EOF parsing YUV4MPEG2 file %s.\n",f);
  exit(1);

}

int spinner=0;
char *spinascii="|/-\\";
void spinnit(void){
  spinner++;
  if(spinner==4)spinner=0;
  fprintf(stderr,"\r%c",spinascii[spinner]);
}

int fetch_and_process_audio(FILE *audio,ogg_page *audiopage,
                            ogg_stream_state *vo,
                            vorbis_dsp_state *vd,
                            vorbis_block *vb,
                            int audioflag){
  static ogg_int64_t samples_sofar=0;
  ogg_packet op;
  int i,j;
  ogg_int64_t beginsample = audio_hz*begin_sec + audio_hz*begin_usec*.000001;
  ogg_int64_t endsample = audio_hz*end_sec + audio_hz*end_usec*.000001;

  while(audio && !audioflag){
    /* process any audio already buffered */
    spinnit();
    if(ogg_stream_pageout(vo,audiopage)>0) return 1;
    if(ogg_stream_eos(vo))return 0;

    {
      /* read and process more audio */
      signed char readbuffer[4096];
      signed char *readptr=readbuffer;
      int toread=4096/2/audio_ch;
      int bytesread=fread(readbuffer,1,toread*2*audio_ch,audio);
      int sampread=bytesread/2/audio_ch;
      float **vorbis_buffer;
      int count=0;

      if(bytesread<=0 ||
         (samples_sofar>=endsample && endsample>0)){
        /* end of file.  this can be done implicitly, but it's
           easier to see here in non-clever fashion.  Tell the
           library we're at end of stream so that it can handle the
           last frame and mark end of stream in the output properly */
        vorbis_analysis_wrote(vd,0);
      }else{
        if(samples_sofar < beginsample){
          if(samples_sofar+sampread > beginsample){
            readptr += (beginsample-samples_sofar)*2*audio_ch;
            sampread += samples_sofar-beginsample;
            samples_sofar = sampread+beginsample;
          }else{
            samples_sofar += sampread;
            sampread = 0;
          }
        }else{
          samples_sofar += sampread;
        }

        if(samples_sofar > endsample && endsample > 0)
          sampread-= (samples_sofar - endsample);

        if(sampread>0){

          vorbis_buffer=vorbis_analysis_buffer(vd,sampread);
          /* uninterleave samples */
          for(i=0;i<sampread;i++){
            for(j=0;j<audio_ch;j++){
              vorbis_buffer[j][i]=((readptr[count+1]<<8)|
                                   (0x00ff&(int)readptr[count]))/32768.f;
              count+=2;
            }
          }

          vorbis_analysis_wrote(vd,sampread);
        }
      }

      while(vorbis_analysis_blockout(vd,vb)==1){

        /* analysis, assume we want to use bitrate management */
        vorbis_analysis(vb,NULL);
        vorbis_bitrate_addblock(vb);

        /* weld packets into the bitstream */
        while(vorbis_bitrate_flushpacket(vd,&op))
          ogg_stream_packetin(vo,&op);

      }
    }
  }

  return audioflag;
}

static int                 frame_state=-1;
static ogg_int64_t         frames=0;
static unsigned char      *yuvframe[3];
static th_ycbcr_buffer     ycbcr;

int fetch_and_process_video_packet(FILE *video,FILE *twopass_file,int passno,
 th_enc_ctx *td,ogg_packet *op){
  int                        ret;
  int                        pic_sz;
  int                        frame_c_w;
  int                        frame_c_h;
  int                        c_w;
  int                        c_h;
  int                        c_sz;
  ogg_int64_t                beginframe;
  ogg_int64_t                endframe;
  spinnit();
  beginframe=(video_fps_n*begin_sec+video_fps_n*begin_usec*.000001)/video_fps_d;
  endframe=(video_fps_n*end_sec+video_fps_n*end_usec*.000001)/video_fps_d;
  if(frame_state==-1){
    /* initialize the double frame buffer */
    yuvframe[0]=(unsigned char *)malloc(y4m_dst_buf_sz);
    yuvframe[1]=(unsigned char *)malloc(y4m_dst_buf_sz);
    yuvframe[2]=(unsigned char *)malloc(y4m_aux_buf_sz);
    frame_state=0;
  }
  pic_sz=pic_w*pic_h;
  frame_c_w=frame_w/dst_c_dec_h;
  frame_c_h=frame_h/dst_c_dec_v;
  c_w=(pic_w+dst_c_dec_h-1)/dst_c_dec_h;
  c_h=(pic_h+dst_c_dec_v-1)/dst_c_dec_v;
  c_sz=c_w*c_h;
  /* read and process more video */
  /* video strategy reads one frame ahead so we know when we're
     at end of stream and can mark last video frame as such
     (vorbis audio has to flush one frame past last video frame
     due to overlap and thus doesn't need this extra work */

  /* have two frame buffers full (if possible) before
     proceeding.  after first pass and until eos, one will
     always be full when we get here */
  for(;frame_state<2 && (frames<endframe || endframe<0);){
    char c,frame[6];
    int ret=fread(frame,1,6,video);
    /* match and skip the frame header */
    if(ret<6)break;
    if(memcmp(frame,"FRAME",5)){
      fprintf(stderr,"Loss of framing in YUV input data\n");
      exit(1);
    }
    if(frame[5]!='\n'){
      int j;
      for(j=0;j<79;j++)
        if(fread(&c,1,1,video)&&c=='\n')break;
      if(j==79){
        fprintf(stderr,"Error parsing YUV frame header\n");
        exit(1);
      }
    }
    /*Read the frame data that needs no conversion.*/
    if(fread(yuvframe[frame_state],1,y4m_dst_buf_read_sz,video)!=
     y4m_dst_buf_read_sz){
      fprintf(stderr,"Error reading YUV frame data.\n");
      exit(1);
    }
    /*Read the frame data that does need conversion.*/
    if(fread(yuvframe[2],1,y4m_aux_buf_read_sz,video)!=y4m_aux_buf_read_sz){
      fprintf(stderr,"Error reading YUV frame data.\n");
      exit(1);
    }
    /*Now convert the just read frame.*/
    (*y4m_convert)(yuvframe[frame_state],yuvframe[2]);
    frames++;
    if(frames>=beginframe)
    frame_state++;
  }
  /* check to see if there are dupes to flush */
  if(th_encode_packetout(td,frame_state<1,op)>0)return 1;
  if(frame_state<1){
    /* can't get here unless YUV4MPEG stream has no video */
    fprintf(stderr,"Video input contains no frames.\n");
    exit(1);
  }
  /* Theora is a one-frame-in,one-frame-out system; submit a frame
     for compression and pull out the packet */
  /* in two-pass mode's second pass, we need to submit first-pass data */
  if(passno==2){
    for(;;){
      static unsigned char buffer[80];
      static int buf_pos;
      int bytes;
      /*Ask the encoder how many bytes it would like.*/
      bytes=th_encode_ctl(td,TH_ENCCTL_2PASS_IN,NULL,0);
      if(bytes<0){
        fprintf(stderr,"Error submitting pass data in second pass.\n");
        exit(1);
      }
      /*If it's got enough, stop.*/
      if(bytes==0)break;
      /*Read in some more bytes, if necessary.*/
      if(bytes>80-buf_pos)bytes=80-buf_pos;
      if(bytes>0&&fread(buffer+buf_pos,1,bytes,twopass_file)<bytes){
        fprintf(stderr,"Could not read frame data from two-pass data file!\n");
        exit(1);
      }
      /*And pass them off.*/
      ret=th_encode_ctl(td,TH_ENCCTL_2PASS_IN,buffer,bytes);
      if(ret<0){
        fprintf(stderr,"Error submitting pass data in second pass.\n");
        exit(1);
      }
      /*If the encoder consumed the whole buffer, reset it.*/
      if(ret>=bytes)buf_pos=0;
      /*Otherwise remember how much it used.*/
      else buf_pos+=ret;
    }
  }
  /*We submit the buffer to the library as if it were padded, but we do not
     actually allocate space for the padding.
    This is okay, because with the 1.0 API the library will never read data from the padded
     region.*/
  ycbcr[0].width=frame_w;
  ycbcr[0].height=frame_h;
  ycbcr[0].stride=pic_w;
  ycbcr[0].data=yuvframe[0]-pic_x-pic_y*pic_w;
  ycbcr[1].width=frame_c_w;
  ycbcr[1].height=frame_c_h;
  ycbcr[1].stride=c_w;
  ycbcr[1].data=yuvframe[0]+pic_sz-(pic_x/dst_c_dec_h)-(pic_y/dst_c_dec_v)*c_w;
  ycbcr[2].width=frame_c_w;
  ycbcr[2].height=frame_c_h;
  ycbcr[2].stride=c_w;
  ycbcr[2].data=ycbcr[1].data+c_sz;
  th_encode_ycbcr_in(td,ycbcr);
  {
    unsigned char *temp=yuvframe[0];
    yuvframe[0]=yuvframe[1];
    yuvframe[1]=temp;
    frame_state--;
  }
  /* in two-pass mode's first pass we need to extract and save the pass data */
  if(passno==1){
    unsigned char *buffer;
    int bytes = th_encode_ctl(td, TH_ENCCTL_2PASS_OUT, &buffer, sizeof(buffer));
    if(bytes<0){
      fprintf(stderr,"Could not read two-pass data from encoder.\n");
      exit(1);
    }
    if(fwrite(buffer,1,bytes,twopass_file)<bytes){
      fprintf(stderr,"Unable to write to two-pass data file.\n");
      exit(1);
    }
    fflush(twopass_file);
  }
  /* if there was only one frame, it's the last in the stream */
  ret = th_encode_packetout(td,frame_state<1,op);
  if(passno==1 && frame_state<1){
    /* need to read the final (summary) packet */
    unsigned char *buffer;
    int bytes = th_encode_ctl(td, TH_ENCCTL_2PASS_OUT, &buffer, sizeof(buffer));
    if(bytes<0){
      fprintf(stderr,"Could not read two-pass summary data from encoder.\n");
      exit(1);
    }
    if(fseek(twopass_file,0,SEEK_SET)<0){
      fprintf(stderr,"Unable to seek in two-pass data file.\n");
      exit(1);
    }
    if(fwrite(buffer,1,bytes,twopass_file)<bytes){
      fprintf(stderr,"Unable to write to two-pass data file.\n");
      exit(1);
    }
    fflush(twopass_file);
  }
  return ret;
}


int fetch_and_process_video(FILE *video,ogg_page *videopage,
 ogg_stream_state *to,th_enc_ctx *td,FILE *twopass_file,int passno,
 int videoflag){
  ogg_packet op;
  int ret;
  /* is there a video page flushed?  If not, work until there is. */
  while(!videoflag){
    if(ogg_stream_pageout(to,videopage)>0) return 1;
    if(ogg_stream_eos(to)) return 0;
    ret=fetch_and_process_video_packet(video,twopass_file,passno,td,&op);
    if(ret<=0)return 0;
    ogg_stream_packetin(to,&op);
  }
  return videoflag;
}

static int ilog(unsigned _v){
  int ret;
  for(ret=0;_v;ret++)_v>>=1;
  return ret;
}

int main(int argc,char *argv[]){
  int c,long_option_index,ret;

  ogg_stream_state to; /* take physical pages, weld into a logical
                           stream of packets */
  ogg_stream_state vo; /* take physical pages, weld into a logical
                           stream of packets */
  ogg_page         og; /* one Ogg bitstream page.  Vorbis packets are inside */
  ogg_packet       op; /* one raw packet of data for decode */

  th_enc_ctx      *td;
  th_info          ti;
  th_comment       tc;

  vorbis_info      vi; /* struct that stores all the static vorbis bitstream
                          settings */
  vorbis_comment   vc; /* struct that stores all the user comments */

  vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
  vorbis_block     vb; /* local working space for packet->PCM decode */

  int speed=-1;
  int audioflag=0;
  int videoflag=0;
  int akbps=0;
  int vkbps=0;
  int soft_target=0;

  ogg_int64_t audio_bytesout=0;
  ogg_int64_t video_bytesout=0;
  double timebase;

  FILE *outfile = stdout;

  FILE *twopass_file = NULL;
  fpos_t video_rewind_pos;
  int twopass=0;
  int passno;

#ifdef _WIN32 /* We need to set stdin/stdout to binary mode. Damn windows. */
  /* if we were reading/writing a file, it would also need to in
     binary mode, eg, fopen("file.wav","wb"); */
  /* Beware the evil ifdef. We avoid these where we can, but this one we
     cannot. Don't add any more, you'll probably go to hell if you do. */
  _setmode( _fileno( stdin ), _O_BINARY );
  _setmode( _fileno( stdout ), _O_BINARY );
#endif

  while((c=getopt_long(argc,argv,optstring,options,&long_option_index))!=EOF){
    switch(c){
    case 'o':
      outfile=fopen(optarg,"wb");
      if(outfile==NULL){
        fprintf(stderr,"Unable to open output file '%s'\n", optarg);
        exit(1);
      }
      break;;

    case 'a':
      audio_q=(float)(atof(optarg)*.099);
      if(audio_q<-.1 || audio_q>1){
        fprintf(stderr,"Illegal audio quality (choose -1 through 10)\n");
        exit(1);
      }
      audio_r=-1;
      break;

    case 'v':
      video_q=(int)rint(6.3*atof(optarg));
      if(video_q<0 || video_q>63){
        fprintf(stderr,"Illegal video quality (choose 0 through 10)\n");
        exit(1);
      }
      break;

    case 'A':
      audio_r=(int)(atof(optarg)*1000);
      if(audio_q<0){
        fprintf(stderr,"Illegal audio quality (choose > 0 please)\n");
        exit(1);
      }
      audio_q=-99;
      break;

    case 'V':
      video_r=(int)rint(atof(optarg)*1000);
      if(video_r<1){
        fprintf(stderr,"Illegal video bitrate (choose > 0 please)\n");
        exit(1);
      }
     break;

    case '\1':
      soft_target=1;
      break;

    case 's':
      video_par_n=(int)rint(atof(optarg));
      break;

    case 'S':
      video_par_d=(int)rint(atof(optarg));
      break;

    case 'f':
      video_fps_n=(int)rint(atof(optarg));
      break;

    case 'F':
      video_fps_d=(int)rint(atof(optarg));
      break;

    case 'c':
      vp3_compatible=1;
      break;

    case 'k':
      keyframe_frequency=rint(atof(optarg));
      if(keyframe_frequency<1 || keyframe_frequency>2147483647){
        fprintf(stderr,"Illegal keyframe frequency\n");
        exit(1);
      }
      break;

    case 'd':
      buf_delay=atoi(optarg);
      if(buf_delay<=0){
        fprintf(stderr,"Illegal buffer delay\n");
        exit(1);
      }
      break;

    case 'z':
      speed=atoi(optarg);
      if(speed<0){
        fprintf(stderr,"Illegal speed level\n");
        exit(1);
      }
      break;

    case 'b':
      {
        char *pos=strchr(optarg,':');
        begin_sec=atol(optarg);
        if(pos){
          char *pos2=strchr(++pos,':');
          begin_sec*=60;
          begin_sec+=atol(pos);
          if(pos2){
            pos2++;
            begin_sec*=60;
            begin_sec+=atol(pos2);
            pos=pos2;
          }
        }else
          pos=optarg;
        pos=strchr(pos,'.');
        if(pos){
          int digits = strlen(++pos);
          begin_usec=atol(pos);
          while(digits++ < 6)
            begin_usec*=10;
        }
      }
      break;
    case 'e':
      {
        char *pos=strchr(optarg,':');
        end_sec=atol(optarg);
        if(pos){
          char *pos2=strchr(++pos,':');
          end_sec*=60;
          end_sec+=atol(pos);
          if(pos2){
            pos2++;
            end_sec*=60;
            end_sec+=atol(pos2);
            pos=pos2;
          }
        }else
          pos=optarg;
        pos=strchr(pos,'.');
        if(pos){
          int digits = strlen(++pos);
          end_usec=atol(pos);
          while(digits++ < 6)
            end_usec*=10;
        }
      }
      break;
    case '\2':
      twopass=3; /* perform both passes */
      twopass_file=tmpfile();
      if(!twopass_file){
        fprintf(stderr,"Unable to open temporary file for twopass data\n");
        exit(1);
      }
      break;
    case '\3':
      twopass=1; /* perform first pass */
      twopass_file=fopen(optarg,"wb");
      if(!twopass_file){
        fprintf(stderr,"Unable to open \'%s\' for twopass data\n",optarg);
        exit(1);
      }
      break;
    case '\4':
      twopass=2; /* perform second pass */
      twopass_file=fopen(optarg,"rb");
      if(!twopass_file){
        fprintf(stderr,"Unable to open twopass data file \'%s\'",optarg);
        exit(1);
      }
      break;

    default:
      usage();
    }
  }

  if(soft_target){
    if(video_r<=0){
      fprintf(stderr,"Soft rate target (--soft-target) requested without a bitrate (-V).\n");
      exit(1);
    }
    if(video_q==-1)
      video_q=0;
  }else{
    if(video_q==-1){
      if(video_r>0)
        video_q=0;
      else
        video_q=48;
    }
  }

  if(keyframe_frequency<=0){
    /*Use a default keyframe frequency of 64 for 1-pass (streaming) mode, and
       256 for two-pass mode.*/
    keyframe_frequency=twopass?256:64;
  }

  while(optind<argc){
    /* assume that anything following the options must be a filename */
    id_file(argv[optind]);
    optind++;
  }

  if(twopass==3){
    /* verify that the input is seekable! */
    if(video){
      if(fseek(video,0,SEEK_CUR)){
        fprintf(stderr,"--two-pass (automatic two-pass) requires the video input\n"
                "to be seekable.  For non-seekable input, encoder_example\n"
                "must be run twice, first with the --first-pass option, then\n"
                "with the --second-pass option.\n\n");
        exit(1);
      }
      if(fgetpos(video,&video_rewind_pos)<0){
        fprintf(stderr,"Unable to determine start position of video data.\n");
        exit(1);
      }
    }
  }

  /* Set up Ogg output stream */
  srand(time(NULL));
  ogg_stream_init(&to,rand()); /* oops, add one ot the above */

  /* initialize Vorbis assuming we have audio to compress. */
  if(audio && twopass!=1){
    ogg_stream_init(&vo,rand());
    vorbis_info_init(&vi);
    if(audio_q>-99)
      ret = vorbis_encode_init_vbr(&vi,audio_ch,audio_hz,audio_q);
    else
      ret = vorbis_encode_init(&vi,audio_ch,audio_hz,-1,
                               (int)(64870*(ogg_int64_t)audio_r>>16),-1);
    if(ret){
      fprintf(stderr,"The Vorbis encoder could not set up a mode according to\n"
              "the requested quality or bitrate.\n\n");
      exit(1);
    }

    vorbis_comment_init(&vc);
    vorbis_analysis_init(&vd,&vi);
    vorbis_block_init(&vd,&vb);
  }

  for(passno=(twopass==3?1:twopass);passno<=(twopass==3?2:twopass);passno++){
    /* Set up Theora encoder */
    if(!video){
      fprintf(stderr,"No video files submitted for compression?\n");
      exit(1);
    }
    /* Theora has a divisible-by-sixteen restriction for the encoded frame size */
    /* scale the picture size up to the nearest /16 and calculate offsets */
    frame_w=pic_w+15&~0xF;
    frame_h=pic_h+15&~0xF;
    /*Force the offsets to be even so that chroma samples line up like we
       expect.*/
    pic_x=frame_w-pic_w>>1&~1;
    pic_y=frame_h-pic_h>>1&~1;
    th_info_init(&ti);
    ti.frame_width=frame_w;
    ti.frame_height=frame_h;
    ti.pic_width=pic_w;
    ti.pic_height=pic_h;
    ti.pic_x=pic_x;
    ti.pic_y=pic_y;
    ti.fps_numerator=video_fps_n;
    ti.fps_denominator=video_fps_d;
    ti.aspect_numerator=video_par_n;
    ti.aspect_denominator=video_par_d;
    ti.colorspace=TH_CS_UNSPECIFIED;
    /*Account for the Ogg page overhead.
      This is 1 byte per 255 for lacing values, plus 26 bytes per 4096 bytes for
       the page header, plus approximately 1/2 byte per packet (not accounted for
       here).*/
    ti.target_bitrate=(int)(64870*(ogg_int64_t)video_r>>16);
    ti.quality=video_q;
    ti.keyframe_granule_shift=ilog(keyframe_frequency-1);
    if(dst_c_dec_h==2){
      if(dst_c_dec_v==2)ti.pixel_fmt=TH_PF_420;
      else ti.pixel_fmt=TH_PF_422;
    }
    else ti.pixel_fmt=TH_PF_444;
    td=th_encode_alloc(&ti);
    th_info_clear(&ti);
    /* setting just the granule shift only allows power-of-two keyframe
       spacing.  Set the actual requested spacing. */
    ret=th_encode_ctl(td,TH_ENCCTL_SET_KEYFRAME_FREQUENCY_FORCE,
     &keyframe_frequency,sizeof(keyframe_frequency-1));
    if(ret<0){
      fprintf(stderr,"Could not set keyframe interval to %d.\n",(int)keyframe_frequency);
    }
    if(vp3_compatible){
      ret=th_encode_ctl(td,TH_ENCCTL_SET_VP3_COMPATIBLE,&vp3_compatible,
       sizeof(vp3_compatible));
      if(ret<0||!vp3_compatible){
        fprintf(stderr,"Could not enable strict VP3 compatibility.\n");
        if(ret>=0){
          fprintf(stderr,"Ensure your source format is supported by VP3.\n");
          fprintf(stderr,
           "(4:2:0 pixel format, width and height multiples of 16).\n");
        }
      }
    }
    if(soft_target){
      /* reverse the rate control flags to favor a 'long time' strategy */
      int arg = TH_RATECTL_CAP_UNDERFLOW;
      ret=th_encode_ctl(td,TH_ENCCTL_SET_RATE_FLAGS,&arg,sizeof(arg));
      if(ret<0)
        fprintf(stderr,"Could not set encoder flags for --soft-target\n");
      /* Default buffer control is overridden on two-pass */
      if(!twopass&&buf_delay<0){
        if((keyframe_frequency*7>>1) > 5*video_fps_n/video_fps_d)
          arg=keyframe_frequency*7>>1;
        else
          arg=5*video_fps_n/video_fps_d;
        ret=th_encode_ctl(td,TH_ENCCTL_SET_RATE_BUFFER,&arg,sizeof(arg));
        if(ret<0)
          fprintf(stderr,"Could not set rate control buffer for --soft-target\n");
      }
    }
    /* set up two-pass if needed */
    if(passno==1){
      unsigned char *buffer;
      int bytes;
      bytes=th_encode_ctl(td,TH_ENCCTL_2PASS_OUT,&buffer,sizeof(buffer));
      if(bytes<0){
        fprintf(stderr,"Could not set up the first pass of two-pass mode.\n");
        fprintf(stderr,"Did you remember to specify an estimated bitrate?\n");
        exit(1);
      }
      /*Perform a seek test to ensure we can overwrite this placeholder data at
         the end; this is better than letting the user sit through a whole
         encode only to find out their pass 1 file is useless at the end.*/
      if(fseek(twopass_file,0,SEEK_SET)<0){
        fprintf(stderr,"Unable to seek in two-pass data file.\n");
        exit(1);
      }
      if(fwrite(buffer,1,bytes,twopass_file)<bytes){
        fprintf(stderr,"Unable to write to two-pass data file.\n");
        exit(1);
      }
      fflush(twopass_file);
    }
    if(passno==2){
      /*Enable the second pass here.
        We make this call just to set the encoder into 2-pass mode, because
         by default enabling two-pass sets the buffer delay to the whole file
         (because there's no way to explicitly request that behavior).
        If we waited until we were actually encoding, it would overwite our
         settings.*/
      if(th_encode_ctl(td,TH_ENCCTL_2PASS_IN,NULL,0)<0){
        fprintf(stderr,"Could not set up the second pass of two-pass mode.\n");
        exit(1);
      }
      if(twopass==3){
        /* 'automatic' second pass */
        if(fsetpos(video,&video_rewind_pos)<0){
          fprintf(stderr,"Could not rewind video input file for second pass!\n");
          exit(1);
        }
        if(fseek(twopass_file,0,SEEK_SET)<0){
          fprintf(stderr,"Unable to seek in two-pass data file.\n");
          exit(1);
        }
        frame_state=0;
        frames=0;
      }
    }
    /*Now we can set the buffer delay if the user requested a non-default one
       (this has to be done after two-pass is enabled).*/
    if(passno!=1&&buf_delay>=0){
      ret=th_encode_ctl(td,TH_ENCCTL_SET_RATE_BUFFER,
       &buf_delay,sizeof(buf_delay));
      if(ret<0){
        fprintf(stderr,"Warning: could not set desired buffer delay.\n");
      }
    }
    /*Speed should also be set after the current encoder mode is established,
       since the available speed levels may change depending.*/
    if(speed>=0){
      int speed_max;
      int ret;
      ret=th_encode_ctl(td,TH_ENCCTL_GET_SPLEVEL_MAX,
       &speed_max,sizeof(speed_max));
      if(ret<0){
        fprintf(stderr,"Warning: could not determine maximum speed level.\n");
        speed_max=0;
      }
      ret=th_encode_ctl(td,TH_ENCCTL_SET_SPLEVEL,&speed,sizeof(speed));
      if(ret<0){
        fprintf(stderr,"Warning: could not set speed level to %i of %i\n",
         speed,speed_max);
        if(speed>speed_max){
          fprintf(stderr,"Setting it to %i instead\n",speed_max);
        }
        ret=th_encode_ctl(td,TH_ENCCTL_SET_SPLEVEL,
         &speed_max,sizeof(speed_max));
        if(ret<0){
          fprintf(stderr,"Warning: could not set speed level to %i of %i\n",
           speed_max,speed_max);
        }
      }
    }
    /* write the bitstream header packets with proper page interleave */
    th_comment_init(&tc);
    /* first packet will get its own page automatically */
    if(th_encode_flushheader(td,&tc,&op)<=0){
      fprintf(stderr,"Internal Theora library error.\n");
      exit(1);
    }
    if(passno!=1){
      ogg_stream_packetin(&to,&op);
      if(ogg_stream_pageout(&to,&og)!=1){
        fprintf(stderr,"Internal Ogg library error.\n");
        exit(1);
      }
      fwrite(og.header,1,og.header_len,outfile);
      fwrite(og.body,1,og.body_len,outfile);
    }
    /* create the remaining theora headers */
    for(;;){
      ret=th_encode_flushheader(td,&tc,&op);
      if(ret<0){
        fprintf(stderr,"Internal Theora library error.\n");
        exit(1);
      }
      else if(!ret)break;
      if(passno!=1)ogg_stream_packetin(&to,&op);
    }
    if(audio && passno!=1){
      ogg_packet header;
      ogg_packet header_comm;
      ogg_packet header_code;
      vorbis_analysis_headerout(&vd,&vc,&header,&header_comm,&header_code);
      ogg_stream_packetin(&vo,&header); /* automatically placed in its own
                                           page */
      if(ogg_stream_pageout(&vo,&og)!=1){
        fprintf(stderr,"Internal Ogg library error.\n");
        exit(1);
      }
      fwrite(og.header,1,og.header_len,outfile);
      fwrite(og.body,1,og.body_len,outfile);
      /* remaining vorbis header packets */
      ogg_stream_packetin(&vo,&header_comm);
      ogg_stream_packetin(&vo,&header_code);
    }
    /* Flush the rest of our headers. This ensures
       the actual data in each stream will start
       on a new page, as per spec. */
    if(passno!=1){
      for(;;){
        int result = ogg_stream_flush(&to,&og);
        if(result<0){
          /* can't get here */
          fprintf(stderr,"Internal Ogg library error.\n");
          exit(1);
        }
        if(result==0)break;
        fwrite(og.header,1,og.header_len,outfile);
        fwrite(og.body,1,og.body_len,outfile);
      }
    }
    if(audio && passno!=1){
      for(;;){
        int result=ogg_stream_flush(&vo,&og);
        if(result<0){
          /* can't get here */
          fprintf(stderr,"Internal Ogg library error.\n");
          exit(1);
        }
        if(result==0)break;
        fwrite(og.header,1,og.header_len,outfile);
        fwrite(og.body,1,og.body_len,outfile);
      }
    }
    /* setup complete.  Raw processing loop */
      switch(passno){
      case 0: case 2:
        fprintf(stderr,"\rCompressing....                                          \n");
        break;
      case 1:
        fprintf(stderr,"\rScanning first pass....                                  \n");
        break;
      }
    for(;;){
      int audio_or_video=-1;
      if(passno==1){
        ogg_packet op;
        int ret=fetch_and_process_video_packet(video,twopass_file,passno,td,&op);
        if(ret<0)break;
        if(op.e_o_s)break; /* end of stream */
        timebase=th_granule_time(td,op.granulepos);
        audio_or_video=1;
      }else{
        double audiotime;
        double videotime;
        ogg_page audiopage;
        ogg_page videopage;
        /* is there an audio page flushed?  If not, fetch one if possible */
        audioflag=fetch_and_process_audio(audio,&audiopage,&vo,&vd,&vb,audioflag);
        /* is there a video page flushed?  If not, fetch one if possible */
        videoflag=fetch_and_process_video(video,&videopage,&to,td,twopass_file,passno,videoflag);
        /* no pages of either?  Must be end of stream. */
        if(!audioflag && !videoflag)break;
        /* which is earlier; the end of the audio page or the end of the
           video page? Flush the earlier to stream */
        audiotime=
        audioflag?vorbis_granule_time(&vd,ogg_page_granulepos(&audiopage)):-1;
        videotime=
        videoflag?th_granule_time(td,ogg_page_granulepos(&videopage)):-1;
        if(!audioflag){
          audio_or_video=1;
        } else if(!videoflag) {
          audio_or_video=0;
        } else {
          if(audiotime<videotime)
            audio_or_video=0;
          else
            audio_or_video=1;
        }
        if(audio_or_video==1){
          /* flush a video page */
          video_bytesout+=fwrite(videopage.header,1,videopage.header_len,outfile);
          video_bytesout+=fwrite(videopage.body,1,videopage.body_len,outfile);
          videoflag=0;
          timebase=videotime;
        }else{
          /* flush an audio page */
          audio_bytesout+=fwrite(audiopage.header,1,audiopage.header_len,outfile);
          audio_bytesout+=fwrite(audiopage.body,1,audiopage.body_len,outfile);
          audioflag=0;
          timebase=audiotime;
        }
      }
      if(timebase > 0){
        int hundredths=(int)(timebase*100-(long)timebase*100);
        int seconds=(long)timebase%60;
        int minutes=((long)timebase/60)%60;
        int hours=(long)timebase/3600;
        if(audio_or_video)vkbps=(int)rint(video_bytesout*8./timebase*.001);
        else akbps=(int)rint(audio_bytesout*8./timebase*.001);
        fprintf(stderr,
                "\r      %d:%02d:%02d.%02d audio: %dkbps video: %dkbps                 ",
                hours,minutes,seconds,hundredths,akbps,vkbps);
      }
    }
    if(video)th_encode_free(td);
  }

  /* clear out state */
  if(audio && twopass!=1){
    ogg_stream_clear(&vo);
    vorbis_block_clear(&vb);
    vorbis_dsp_clear(&vd);
    vorbis_comment_clear(&vc);
    vorbis_info_clear(&vi);
    if(audio!=stdin)fclose(audio);
  }
  if(video){
    ogg_stream_clear(&to);
    th_comment_clear(&tc);
    if(video!=stdin)fclose(video);
  }

  if(outfile && outfile!=stdout)fclose(outfile);
  if(twopass_file)fclose(twopass_file);

  fprintf(stderr,"\r   \ndone.\n\n");

  return(0);

}
