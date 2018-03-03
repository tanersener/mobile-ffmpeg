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

  function: example SDL player application; plays Ogg Theora files (with
            optional Vorbis audio second stream)
  last mod: $Id: player_example.c 16551 2009-09-09 17:53:13Z gmaxwell $

 ********************************************************************/

/* far more complex than most Ogg 'example' programs.  The complexity
   of maintaining A/V sync is pretty much unavoidable.  It's necessary
   to actually have audio/video playback to make the hard audio clock
   sync actually work.  If there's audio playback, there might as well
   be simple video playback as well...

   A simple 'demux and write back streams' would have been easier,
   it's true. */

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef _REENTRANT
# define _REENTRANT
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include "theora/theoradec.h"
#include "vorbis/codec.h"
#include <SDL.h>

/* yes, this makes us OSS-specific for now. None of SDL, libao, libao2
   give us any way to determine hardware timing, and since the
   hard/kernel buffer is going to be most of or > a second, that's
   just a little bit important */
#if defined(__FreeBSD__)
#include <machine/soundcard.h>
#define AUDIO_DEVICE "/dev/audio"
#elif defined(__NetBSD__) || defined(__OpenBSD__)
#include <soundcard.h>
#define AUDIO_DEVICE "/dev/audio"
#else
#include <sys/soundcard.h>
#define AUDIO_DEVICE "/dev/dsp"
#endif
#include <sys/ioctl.h>

/* Helper; just grab some more compressed bitstream and sync it for
   page extraction */
int buffer_data(FILE *in,ogg_sync_state *oy){
  char *buffer=ogg_sync_buffer(oy,4096);
  int bytes=fread(buffer,1,4096,in);
  ogg_sync_wrote(oy,bytes);
  return(bytes);
}

/* never forget that globals are a one-way ticket to Hell */
/* Ogg and codec state for demux/decode */
ogg_sync_state   oy;
ogg_page         og;
ogg_stream_state vo;
ogg_stream_state to;
th_info      ti;
th_comment   tc;
th_dec_ctx       *td;
th_setup_info    *ts;
vorbis_info      vi;
vorbis_dsp_state vd;
vorbis_block     vb;
vorbis_comment   vc;
th_pixel_fmt     px_fmt;

int              theora_p=0;
int              vorbis_p=0;
int              stateflag=0;

/* SDL Video playback structures */
SDL_Surface *screen;
SDL_Overlay *yuv_overlay;
SDL_Rect rect;

/* single frame video buffering */
int          videobuf_ready=0;
ogg_int64_t  videobuf_granulepos=-1;
double       videobuf_time=0;

/* single audio fragment audio buffering */
int          audiobuf_fill=0;
int          audiobuf_ready=0;
ogg_int16_t *audiobuf;
ogg_int64_t  audiobuf_granulepos=0; /* time position of last sample */

/* audio / video synchronization tracking:

Since this will make it to Google at some point and lots of people
search for how to do this, a quick rundown of a practical A/V sync
strategy under Linux [the UNIX where Everything Is Hard].  Naturally,
this works on other platforms using OSS for sound as well.

In OSS, we don't have reliable access to any precise information on
the exact current playback position (that, of course would have been
too easy; the kernel folks like to keep us app people working hard
doing simple things that should have been solved once and abstracted
long ago).  Hopefully ALSA solves this a little better; we'll probably
use that once ALSA is the standard in the stable kernel.

We can't use the system clock for a/v sync because audio is hard
synced to its own clock, and both the system and audio clocks suffer
from wobble, drift, and a lack of accuracy that can be guaranteed to
add a reliable percent or so of error.  After ten seconds, that's
100ms.  We can't drift by half a second every minute.

Although OSS can't generally tell us where the audio playback pointer
is, we do know that if we work in complete audio fragments and keep
the kernel buffer full, a blocking select on the audio buffer will
give us a writable fragment immediately after playback finishes with
it.  We assume at that point that we know the exact number of bytes in
the kernel buffer that have not been played (total fragments minus
one) and calculate clock drift between audio and system then (and only
then).  Damp the sync correction fraction, apply, and walla: A
reliable A/V clock that even works if it's interrupted. */

long         audiofd_totalsize=-1;
int          audiofd_fragsize;      /* read and write only complete fragments
                                       so that SNDCTL_DSP_GETOSPACE is
                                       accurate immediately after a bank
                                       switch */
int          audiofd=-1;
ogg_int64_t  audiofd_timer_calibrate=-1;

static void open_audio(){
  audio_buf_info info;
  int format=AFMT_S16_NE; /* host endian */
  int channels=vi.channels;
  int rate=vi.rate;
  int ret;

  audiofd=open(AUDIO_DEVICE,O_RDWR);
  if(audiofd<0){
    fprintf(stderr,"Could not open audio device " AUDIO_DEVICE ".\n");
    exit(1);
  }

  ret=ioctl(audiofd,SNDCTL_DSP_SETFMT,&format);
  if(ret){
    fprintf(stderr,"Could not set 16 bit host-endian playback\n");
    exit(1);
  }

  ret=ioctl(audiofd,SNDCTL_DSP_CHANNELS,&channels);
  if(ret){
    fprintf(stderr,"Could not set %d channel playback\n",channels);
    exit(1);
  }

  ret=ioctl(audiofd,SNDCTL_DSP_SPEED,&rate);
  if(ret){
    fprintf(stderr,"Could not set %d Hz playback\n",rate);
    exit(1);
  }

  ioctl(audiofd,SNDCTL_DSP_GETOSPACE,&info);
  audiofd_fragsize=info.fragsize;
  audiofd_totalsize=info.fragstotal*info.fragsize;

  audiobuf=malloc(audiofd_fragsize);
}

static void audio_close(void){
  if(audiofd>-1){
    ioctl(audiofd,SNDCTL_DSP_RESET,NULL);
    close(audiofd);
    free(audiobuf);
  }
}

/* call this only immediately after unblocking from a full kernel
   having a newly empty fragment or at the point of DMA restart */
void audio_calibrate_timer(int restart){
  struct timeval tv;
  ogg_int64_t current_sample;
  ogg_int64_t new_time;

  gettimeofday(&tv,0);
  new_time=tv.tv_sec*1000+tv.tv_usec/1000;

  if(restart){
    current_sample=audiobuf_granulepos-audiobuf_fill/2/vi.channels;
  }else
    current_sample=audiobuf_granulepos-
      (audiobuf_fill+audiofd_totalsize-audiofd_fragsize)/2/vi.channels;

  new_time-=1000*current_sample/vi.rate;

  audiofd_timer_calibrate=new_time;
}

/* get relative time since beginning playback, compensating for A/V
   drift */
double get_time(){
  static ogg_int64_t last=0;
  static ogg_int64_t up=0;
  ogg_int64_t now;
  struct timeval tv;

  gettimeofday(&tv,0);
  now=tv.tv_sec*1000+tv.tv_usec/1000;

  if(audiofd_timer_calibrate==-1)audiofd_timer_calibrate=last=now;

  if(audiofd<0){
    /* no audio timer to worry about, we can just use the system clock */
    /* only one complication: If the process is suspended, we should
       reset timing to account for the gap in play time.  Do it the
       easy/hack way */
    if(now-last>1000)audiofd_timer_calibrate+=(now-last);
    last=now;
  }

  if(now-up>200){
    double timebase=(now-audiofd_timer_calibrate)*.001;
    int hundredths=timebase*100-(long)timebase*100;
    int seconds=(long)timebase%60;
    int minutes=((long)timebase/60)%60;
    int hours=(long)timebase/3600;

    fprintf(stderr,"   Playing: %d:%02d:%02d.%02d                       \r",
            hours,minutes,seconds,hundredths);
    up=now;
  }

  return (now-audiofd_timer_calibrate)*.001;

}

/* write a fragment to the OSS kernel audio API, but only if we can
   stuff in a whole fragment without blocking */
void audio_write_nonblocking(void){

  if(audiobuf_ready){
    audio_buf_info info;
    long bytes;

    ioctl(audiofd,SNDCTL_DSP_GETOSPACE,&info);
    bytes=info.bytes;
    if(bytes>=audiofd_fragsize){
      if(bytes==audiofd_totalsize)audio_calibrate_timer(1);

      while(1){
        bytes=write(audiofd,audiobuf+(audiofd_fragsize-audiobuf_fill),
                    audiofd_fragsize);

        if(bytes>0){

          if(bytes!=audiobuf_fill){
            /* shouldn't actually be possible... but eh */
            audiobuf_fill-=bytes;
          }else
            break;
        }
      }

      audiobuf_fill=0;
      audiobuf_ready=0;

    }
  }
}

/* clean quit on Ctrl-C for SDL and thread shutdown as per SDL example
   (we don't use any threads, but libSDL does) */
int got_sigint=0;
static void sigint_handler (int signal) {
  got_sigint = 1;
}

static void open_video(void){
  int w;
  int h;
  w=(ti.pic_x+ti.frame_width+1&~1)-(ti.pic_x&~1);
  h=(ti.pic_y+ti.frame_height+1&~1)-(ti.pic_y&~1);
  if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
    fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
    exit(1);
  }

  screen = SDL_SetVideoMode(w, h, 0, SDL_SWSURFACE);
  if ( screen == NULL ) {
    fprintf(stderr, "Unable to set %dx%d video: %s\n",
            w,h,SDL_GetError());
    exit(1);
  }

  if (px_fmt==TH_PF_422)
    yuv_overlay = SDL_CreateYUVOverlay(w, h,
                                     SDL_YUY2_OVERLAY,
                                     screen);
  else
    yuv_overlay = SDL_CreateYUVOverlay(w, h,
                                     SDL_YV12_OVERLAY,
                                     screen);
  
  if ( yuv_overlay == NULL ) {
    fprintf(stderr, "SDL: Couldn't create SDL_yuv_overlay: %s\n",
            SDL_GetError());
    exit(1);
  }
  rect.x = 0;
  rect.y = 0;
  rect.w = w;
  rect.h = h;

  SDL_DisplayYUVOverlay(yuv_overlay, &rect);
}

static void video_write(void){
  int i;
  th_ycbcr_buffer yuv;
  int y_offset, uv_offset;
  th_decode_ycbcr_out(td,yuv);
  /* Lock SDL_yuv_overlay */
  if ( SDL_MUSTLOCK(screen) ) {
    if ( SDL_LockSurface(screen) < 0 ) return;
  }
  if (SDL_LockYUVOverlay(yuv_overlay) < 0) return;

  /* let's draw the data on a SDL screen (*screen) */
  /* deal with border stride */
  /* reverse u and v for SDL */
  /* and crop input properly, respecting the encoded frame rect */
  /* problems may exist for odd frame rect for some encodings */

  y_offset=(ti.pic_x&~1)+yuv[0].stride*(ti.pic_y&~1);

  if (px_fmt==TH_PF_422) {
    uv_offset=(ti.pic_x/2)+(yuv[1].stride)*(ti.pic_y);
    /* SDL doesn't have a planar 4:2:2 */ 
    for(i=0;i<yuv_overlay->h;i++) {
      int j;
      char *in_y  = (char *)yuv[0].data+y_offset+yuv[0].stride*i;
      char *out = (char *)(yuv_overlay->pixels[0]+yuv_overlay->pitches[0]*i);
      for (j=0;j<yuv_overlay->w;j++)
        out[j*2] = in_y[j];
      char *in_u  = (char *)yuv[1].data+uv_offset+yuv[1].stride*i;
      char *in_v  = (char *)yuv[2].data+uv_offset+yuv[2].stride*i;
      for (j=0;j<yuv_overlay->w>>1;j++) {
        out[j*4+1] = in_u[j];
        out[j*4+3] = in_v[j];
      }
    }
  } else {
    uv_offset=(ti.pic_x/2)+(yuv[1].stride)*(ti.pic_y/2);
    for(i=0;i<yuv_overlay->h;i++)
      memcpy(yuv_overlay->pixels[0]+yuv_overlay->pitches[0]*i,
           yuv[0].data+y_offset+yuv[0].stride*i,
           yuv_overlay->w);
    for(i=0;i<yuv_overlay->h/2;i++){
      memcpy(yuv_overlay->pixels[1]+yuv_overlay->pitches[1]*i,
           yuv[2].data+uv_offset+yuv[2].stride*i,
           yuv_overlay->w/2);
      memcpy(yuv_overlay->pixels[2]+yuv_overlay->pitches[2]*i,
           yuv[1].data+uv_offset+yuv[1].stride*i,
           yuv_overlay->w/2);
    }
  }

  /* Unlock SDL_yuv_overlay */
  if ( SDL_MUSTLOCK(screen) ) {
    SDL_UnlockSurface(screen);
  }
  SDL_UnlockYUVOverlay(yuv_overlay);


  /* Show, baby, show! */
  SDL_DisplayYUVOverlay(yuv_overlay, &rect);

}
/* dump the theora (or vorbis) comment header */
static int dump_comments(th_comment *tc){
  int i, len;
  char *value;
  FILE *out=stdout;

  fprintf(out,"Encoded by %s\n",tc->vendor);
  if(tc->comments){
    fprintf(out, "theora comment header:\n");
    for(i=0;i<tc->comments;i++){
      if(tc->user_comments[i]){
        len=tc->comment_lengths[i];
        value=malloc(len+1);
        memcpy(value,tc->user_comments[i],len);
        value[len]='\0';
        fprintf(out, "\t%s\n", value);
        free(value);
      }
    }
  }
  return(0);
}

/* Report the encoder-specified colorspace for the video, if any.
   We don't actually make use of the information in this example;
   a real player should attempt to perform color correction for
   whatever display device it supports. */
static void report_colorspace(th_info *ti)
{
    switch(ti->colorspace){
      case TH_CS_UNSPECIFIED:
        /* nothing to report */
        break;;
      case TH_CS_ITU_REC_470M:
        fprintf(stderr,"  encoder specified ITU Rec 470M (NTSC) color.\n");
        break;;
      case TH_CS_ITU_REC_470BG:
        fprintf(stderr,"  encoder specified ITU Rec 470BG (PAL) color.\n");
        break;;
      default:
        fprintf(stderr,"warning: encoder specified unknown colorspace (%d).\n",
            ti->colorspace);
        break;;
    }
}

/* helper: push a page into the appropriate steam */
/* this can be done blindly; a stream won't accept a page
                that doesn't belong to it */
static int queue_page(ogg_page *page){
  if(theora_p)ogg_stream_pagein(&to,page);
  if(vorbis_p)ogg_stream_pagein(&vo,page);
  return 0;
}

static void usage(void){
  fprintf(stderr,
          "Usage: player_example <file.ogv>\n"
          "input is read from stdin if no file is passed on the command line\n"
          "\n"
  );
}

int main(int argc,char *const *argv){

  int pp_level_max;
  int pp_level;
  int pp_inc;
  int i,j;
  ogg_packet op;

  FILE *infile = stdin;

  int frames = 0;
  int dropped = 0;

#ifdef _WIN32 /* We need to set stdin/stdout to binary mode. Damn windows. */
  /* Beware the evil ifdef. We avoid these where we can, but this one we
     cannot. Don't add any more, you'll probably go to hell if you do. */
  _setmode( _fileno( stdin ), _O_BINARY );
#endif

  /* open the input file if any */
  if(argc==2){
    infile=fopen(argv[1],"rb");
    if(infile==NULL){
      fprintf(stderr,"Unable to open '%s' for playback.\n", argv[1]);
      exit(1);
    }
  }
  if(argc>2){
      usage();
      exit(1);
  }

  /* start up Ogg stream synchronization layer */
  ogg_sync_init(&oy);

  /* init supporting Vorbis structures needed in header parsing */
  vorbis_info_init(&vi);
  vorbis_comment_init(&vc);

  /* init supporting Theora structures needed in header parsing */
  th_comment_init(&tc);
  th_info_init(&ti);

  /* Ogg file open; parse the headers */
  /* Only interested in Vorbis/Theora streams */
  while(!stateflag){
    int ret=buffer_data(infile,&oy);
    if(ret==0)break;
    while(ogg_sync_pageout(&oy,&og)>0){
      ogg_stream_state test;

      /* is this a mandated initial header? If not, stop parsing */
      if(!ogg_page_bos(&og)){
        /* don't leak the page; get it into the appropriate stream */
        queue_page(&og);
        stateflag=1;
        break;
      }

      ogg_stream_init(&test,ogg_page_serialno(&og));
      ogg_stream_pagein(&test,&og);
      ogg_stream_packetout(&test,&op);


      /* identify the codec: try theora */
      if(!theora_p && th_decode_headerin(&ti,&tc,&ts,&op)>=0){
        /* it is theora */
        memcpy(&to,&test,sizeof(test));
        theora_p=1;
      }else if(!vorbis_p && vorbis_synthesis_headerin(&vi,&vc,&op)>=0){
        /* it is vorbis */
        memcpy(&vo,&test,sizeof(test));
        vorbis_p=1;
      }else{
        /* whatever it is, we don't care about it */
        ogg_stream_clear(&test);
      }
    }
    /* fall through to non-bos page parsing */
  }

  /* we're expecting more header packets. */
  while((theora_p && theora_p<3) || (vorbis_p && vorbis_p<3)){
    int ret;

    /* look for further theora headers */
    while(theora_p && (theora_p<3) && (ret=ogg_stream_packetout(&to,&op))){
      if(ret<0){
        fprintf(stderr,"Error parsing Theora stream headers; "
         "corrupt stream?\n");
        exit(1);
      }
      if(!th_decode_headerin(&ti,&tc,&ts,&op)){
        fprintf(stderr,"Error parsing Theora stream headers; "
         "corrupt stream?\n");
        exit(1);
      }
      theora_p++;
    }

    /* look for more vorbis header packets */
    while(vorbis_p && (vorbis_p<3) && (ret=ogg_stream_packetout(&vo,&op))){
      if(ret<0){
        fprintf(stderr,"Error parsing Vorbis stream headers; corrupt stream?\n");
        exit(1);
      }
      if(vorbis_synthesis_headerin(&vi,&vc,&op)){
        fprintf(stderr,"Error parsing Vorbis stream headers; corrupt stream?\n");
        exit(1);
      }
      vorbis_p++;
      if(vorbis_p==3)break;
    }

    /* The header pages/packets will arrive before anything else we
       care about, or the stream is not obeying spec */

    if(ogg_sync_pageout(&oy,&og)>0){
      queue_page(&og); /* demux into the appropriate stream */
    }else{
      int ret=buffer_data(infile,&oy); /* someone needs more data */
      if(ret==0){
        fprintf(stderr,"End of file while searching for codec headers.\n");
        exit(1);
      }
    }
  }

  /* and now we have it all.  initialize decoders */
  if(theora_p){
    td=th_decode_alloc(&ti,ts);
    printf("Ogg logical stream %lx is Theora %dx%d %.02f fps",
           to.serialno,ti.pic_width,ti.pic_height,
           (double)ti.fps_numerator/ti.fps_denominator);
    px_fmt=ti.pixel_fmt;
    switch(ti.pixel_fmt){
      case TH_PF_420: printf(" 4:2:0 video\n"); break;
      case TH_PF_422: printf(" 4:2:2 video\n"); break;
      case TH_PF_444: printf(" 4:4:4 video\n"); break;
      case TH_PF_RSVD:
      default:
       printf(" video\n  (UNKNOWN Chroma sampling!)\n");
       break;
    }
    if(ti.pic_width!=ti.frame_width || ti.pic_height!=ti.frame_height)
      printf("  Frame content is %dx%d with offset (%d,%d).\n",
           ti.frame_width, ti.frame_height, ti.pic_x, ti.pic_y);
    report_colorspace(&ti);
    dump_comments(&tc);
    th_decode_ctl(td,TH_DECCTL_GET_PPLEVEL_MAX,&pp_level_max,
     sizeof(pp_level_max));
    pp_level=pp_level_max;
    th_decode_ctl(td,TH_DECCTL_SET_PPLEVEL,&pp_level,sizeof(pp_level));
    pp_inc=0;

    /*{
      int arg = 0xffff;
      th_decode_ctl(td,TH_DECCTL_SET_TELEMETRY_MBMODE,&arg,sizeof(arg));
      th_decode_ctl(td,TH_DECCTL_SET_TELEMETRY_MV,&arg,sizeof(arg));
      th_decode_ctl(td,TH_DECCTL_SET_TELEMETRY_QI,&arg,sizeof(arg));
      arg=10;
      th_decode_ctl(td,TH_DECCTL_SET_TELEMETRY_BITS,&arg,sizeof(arg));
    }*/
  }else{
    /* tear down the partial theora setup */
    th_info_clear(&ti);
    th_comment_clear(&tc);
  }
  
  th_setup_free(ts);
  
  if(vorbis_p){
    vorbis_synthesis_init(&vd,&vi);
    vorbis_block_init(&vd,&vb);
    fprintf(stderr,"Ogg logical stream %lx is Vorbis %d channel %ld Hz audio.\n",
            vo.serialno,vi.channels,vi.rate);
  }else{
    /* tear down the partial vorbis setup */
    vorbis_info_clear(&vi);
    vorbis_comment_clear(&vc);
  }

  /* open audio */
  if(vorbis_p)open_audio();

  /* open video */
  if(theora_p)open_video();

  /* install signal handler as SDL clobbered the default */
  signal (SIGINT, sigint_handler);

  /* on to the main decode loop.  We assume in this example that audio
     and video start roughly together, and don't begin playback until
     we have a start frame for both.  This is not necessarily a valid
     assumption in Ogg A/V streams! It will always be true of the
     example_encoder (and most streams) though. */

  stateflag=0; /* playback has not begun */
  while(!got_sigint){

    /* we want a video and audio frame ready to go at all times.  If
       we have to buffer incoming, buffer the compressed data (ie, let
       ogg do the buffering) */
    while(vorbis_p && !audiobuf_ready){
      int ret;
      float **pcm;

      /* if there's pending, decoded audio, grab it */
      if((ret=vorbis_synthesis_pcmout(&vd,&pcm))>0){
        int count=audiobuf_fill/2;
        int maxsamples=(audiofd_fragsize-audiobuf_fill)/2/vi.channels;
        for(i=0;i<ret && i<maxsamples;i++)
          for(j=0;j<vi.channels;j++){
            int val=rint(pcm[j][i]*32767.f);
            if(val>32767)val=32767;
            if(val<-32768)val=-32768;
            audiobuf[count++]=val;
          }
        vorbis_synthesis_read(&vd,i);
        audiobuf_fill+=i*vi.channels*2;
        if(audiobuf_fill==audiofd_fragsize)audiobuf_ready=1;
        if(vd.granulepos>=0)
          audiobuf_granulepos=vd.granulepos-ret+i;
        else
          audiobuf_granulepos+=i;

      }else{

        /* no pending audio; is there a pending packet to decode? */
        if(ogg_stream_packetout(&vo,&op)>0){
          if(vorbis_synthesis(&vb,&op)==0) /* test for success! */
            vorbis_synthesis_blockin(&vd,&vb);
        }else   /* we need more data; break out to suck in another page */
          break;
      }
    }

    while(theora_p && !videobuf_ready){
      /* theora is one in, one out... */
      if(ogg_stream_packetout(&to,&op)>0){

        if(pp_inc){
          pp_level+=pp_inc;
          th_decode_ctl(td,TH_DECCTL_SET_PPLEVEL,&pp_level,
           sizeof(pp_level));
          pp_inc=0;
        }
        /*HACK: This should be set after a seek or a gap, but we might not have
           a granulepos for the first packet (we only have them for the last
           packet on a page), so we just set it as often as we get it.
          To do this right, we should back-track from the last packet on the
           page and compute the correct granulepos for the first packet after
           a seek or a gap.*/
        if(op.granulepos>=0){
          th_decode_ctl(td,TH_DECCTL_SET_GRANPOS,&op.granulepos,
           sizeof(op.granulepos));
        }
        if(th_decode_packetin(td,&op,&videobuf_granulepos)==0){
          videobuf_time=th_granule_time(td,videobuf_granulepos);
          frames++;

          /* is it already too old to be useful?  This is only actually
             useful cosmetically after a SIGSTOP.  Note that we have to
             decode the frame even if we don't show it (for now) due to
             keyframing.  Soon enough libtheora will be able to deal
             with non-keyframe seeks.  */

          if(videobuf_time>=get_time())
            videobuf_ready=1;
          else{
            /*If we are too slow, reduce the pp level.*/
            pp_inc=pp_level>0?-1:0;
            dropped++;
          }
        }

      }else
        break;
    }

    if(!videobuf_ready && !audiobuf_ready && feof(infile))break;

    if(!videobuf_ready || !audiobuf_ready){
      /* no data yet for somebody.  Grab another page */
      buffer_data(infile,&oy);
      while(ogg_sync_pageout(&oy,&og)>0){
        queue_page(&og);
      }
    }

    /* If playback has begun, top audio buffer off immediately. */
    if(stateflag) audio_write_nonblocking();

    /* are we at or past time for this video frame? */
    if(stateflag && videobuf_ready && videobuf_time<=get_time()){
      video_write();
      videobuf_ready=0;
    }

    if(stateflag &&
       (audiobuf_ready || !vorbis_p) &&
       (videobuf_ready || !theora_p) &&
       !got_sigint){
      /* we have an audio frame ready (which means the audio buffer is
         full), it's not time to play video, so wait until one of the
         audio buffer is ready or it's near time to play video */

      /* set up select wait on the audiobuffer and a timeout for video */
      struct timeval timeout;
      fd_set writefs;
      fd_set empty;
      int n=0;

      FD_ZERO(&writefs);
      FD_ZERO(&empty);
      if(audiofd>=0){
        FD_SET(audiofd,&writefs);
        n=audiofd+1;
      }

      if(theora_p){
        double tdiff;
        long milliseconds;
        tdiff=videobuf_time-get_time();
        /*If we have lots of extra time, increase the post-processing level.*/
        if(tdiff>ti.fps_denominator*0.25/ti.fps_numerator){
          pp_inc=pp_level<pp_level_max?1:0;
        }
        else if(tdiff<ti.fps_denominator*0.05/ti.fps_numerator){
          pp_inc=pp_level>0?-1:0;
        }
        milliseconds=tdiff*1000-5;
        if(milliseconds>500)milliseconds=500;
        if(milliseconds>0){
          timeout.tv_sec=milliseconds/1000;
          timeout.tv_usec=(milliseconds%1000)*1000;

          n=select(n,&empty,&writefs,&empty,&timeout);
          if(n)audio_calibrate_timer(0);
        }
      }else{
        select(n,&empty,&writefs,&empty,NULL);
      }
    }

    /* if our buffers either don't exist or are ready to go,
       we can begin playback */
    if((!theora_p || videobuf_ready) &&
       (!vorbis_p || audiobuf_ready))stateflag=1;
    /* same if we've run out of input */
    if(feof(infile))stateflag=1;

  }

  /* tear it all down */

  audio_close();
  SDL_Quit();

  if(vorbis_p){
    ogg_stream_clear(&vo);
    vorbis_block_clear(&vb);
    vorbis_dsp_clear(&vd);
    vorbis_comment_clear(&vc);
    vorbis_info_clear(&vi);
  }
  if(theora_p){
    ogg_stream_clear(&to);
    th_decode_free(td);
    th_comment_clear(&tc);
    th_info_clear(&ti);
  }
  ogg_sync_clear(&oy);

  if(infile && infile!=stdin)fclose(infile);

  fprintf(stderr,
          "\r                                                             \r");
  fprintf(stderr, "%d frames", frames);
  if (dropped) fprintf(stderr, " (%d dropped)", dropped);
  fprintf(stderr, "\n");
  fprintf(stderr, "\nDone.\n");

  return(0);

}
