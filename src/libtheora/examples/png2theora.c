/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2009,2009           *
 * by the Xiph.Org Foundation and contributors http://www.xiph.org/ *
 *                                                                  *
 ********************************************************************

  function: example encoder application; makes an Ogg Theora
            file from a sequence of png images
  last mod: $Id: png2theora.c 16503 2009-08-22 18:14:02Z giles $
             based on code from Vegard Nossum

 ********************************************************************/

#define _FILE_OFFSET_BITS 64

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <libgen.h>
#include <sys/types.h>
#include <dirent.h>

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <png.h>
#include <ogg/ogg.h>
#include "theora/theoraenc.h"

#define PROGRAM_NAME  "png2theora"
#define PROGRAM_VERSION  "1.1"

static const char *option_output = NULL;
static int video_fps_numerator = 24;
static int video_fps_denominator = 1;
static int video_aspect_numerator = 0;
static int video_aspect_denominator = 0;
static int video_rate = -1;
static int video_quality = -1;
ogg_uint32_t keyframe_frequency=0;
int buf_delay=-1;
int vp3_compatible=0;
static int chroma_format = TH_PF_420;

static FILE *twopass_file = NULL;
static  int twopass=0;
static  int passno;

static FILE *ogg_fp = NULL;
static ogg_stream_state ogg_os;
static ogg_packet op;   
static ogg_page og;
    
static th_enc_ctx      *td;
static th_info          ti;

static char *input_filter;

const char *optstring = "o:hv:\4:\2:V:s:S:f:F:ck:d:\1\2\3\4\5\6";
struct option options [] = {
 {"output",required_argument,NULL,'o'},
 {"help",no_argument,NULL,'h'},
 {"chroma-444",no_argument,NULL,'\5'},
 {"chroma-422",no_argument,NULL,'\6'},
 {"video-rate-target",required_argument,NULL,'V'},
 {"video-quality",required_argument,NULL,'v'},
 {"aspect-numerator",required_argument,NULL,'s'},
 {"aspect-denominator",required_argument,NULL,'S'},
 {"framerate-numerator",required_argument,NULL,'f'},
 {"framerate-denominator",required_argument,NULL,'F'},
 {"vp3-compatible",no_argument,NULL,'c'},
 {"soft-target",no_argument,NULL,'\1'},
 {"keyframe-freq",required_argument,NULL,'k'},
 {"buf-delay",required_argument,NULL,'d'},
 {"two-pass",no_argument,NULL,'\2'},
 {"first-pass",required_argument,NULL,'\3'}, 
 {"second-pass",required_argument,NULL,'\4'},  
 {NULL,0,NULL,0}
};

static void usage(void){
  fprintf(stderr,
          "%s %s\n"
          "Usage: %s [options] <input>\n\n"
          "The input argument uses C printf format to represent a list of files,\n"
          "  i.e. file-%%06d.png to look for files file000001.png to file9999999.png \n\n"
          "Options: \n\n"
          "  -o --output <filename.ogv>      file name for encoded output (required);\n"
          "  -v --video-quality <n>          Theora quality selector fro 0 to 10\n"
          "                                  (0 yields smallest files but lowest\n"
          "                                  video quality. 10 yields highest\n"
          "                                  fidelity but large files)\n\n"
          "  -V --video-rate-target <n>      bitrate target for Theora video\n\n"
          "     --soft-target                Use a large reservoir and treat the rate\n"
          "                                  as a soft target; rate control is less\n"
          "                                  strict but resulting quality is usually\n"
          "                                  higher/smoother overall. Soft target also\n"
          "                                  allows an optional -v setting to specify\n"
          "                                  a minimum allowed quality.\n\n"
          "     --two-pass                   Compress input using two-pass rate control\n"
          "                                  This option performs both passes automatically.\n\n"
          "     --first-pass <filename>      Perform first-pass of a two-pass rate\n"
          "                                  controlled encoding, saving pass data to\n"
          "                                  <filename> for a later second pass\n\n"
          "     --second-pass <filename>     Perform second-pass of a two-pass rate\n"
          "                                  controlled encoding, reading first-pass\n"
          "                                  data from <filename>.  The first pass\n"
          "                                  data must come from a first encoding pass\n"
          "                                  using identical input video to work\n"
          "                                  properly.\n\n"
          "   -k --keyframe-freq <n>         Keyframe frequency\n"
          "   -d --buf-delay <n>             Buffer delay (in frames). Longer delays\n"
          "                                  allow smoother rate adaptation and provide\n"
          "                                  better overall quality, but require more\n"
          "                                  client side buffering and add latency. The\n"
          "                                  default value is the keyframe interval for\n"
          "                                  one-pass encoding (or somewhat larger if\n"
          "                                  --soft-target is used) and infinite for\n"
          "                                  two-pass encoding.\n"
          "  --chroma-444                    Use 4:4:4 chroma subsampling\n"
          "  --chroma-422                    Use 4:2:2 chroma subsampling\n"
          "                                  (4:2:0 is default)\n\n" 
          "  -s --aspect-numerator <n>       Aspect ratio numerator, default is 0\n"
          "  -S --aspect-denominator <n>     Aspect ratio denominator, default is 0\n"
          "  -f --framerate-numerator <n>    Frame rate numerator\n"
          "  -F --framerate-denominator <n>  Frame rate denominator\n"
          "                                  The frame rate nominator divided by this\n"
          "                                  determines the frame rate in units per tick\n"
          ,PROGRAM_NAME, PROGRAM_VERSION, PROGRAM_NAME
  );
  exit(0);
}

#ifdef WIN32
int
alphasort (const void *a, const void *b)
{
  return strcoll ((*(const struct dirent **) a)->d_name,
                  (*(const struct dirent **) b)->d_name);
}

int
scandir (const char *dir, struct dirent ***namelist,
         int (*select)(const struct dirent *), int (*compar)(const void *, const void *))
{
  DIR *d;
  struct dirent *entry;
  register int i=0;
  size_t entrysize;

  if ((d=opendir(dir)) == NULL)
    return(-1);

  *namelist=NULL;
  while ((entry=readdir(d)) != NULL)
  {
    if (select == NULL || (select != NULL && (*select)(entry)))
    {
      *namelist=(struct dirent **)realloc((void *)(*namelist),
                 (size_t)((i+1)*sizeof(struct dirent *)));
      if (*namelist == NULL) return(-1);
      entrysize=sizeof(struct dirent)-sizeof(entry->d_name)+strlen(entry->d_name)+1;
      (*namelist)[i]=(struct dirent *)malloc(entrysize);
      if ((*namelist)[i] == NULL) return(-1);
        memcpy((*namelist)[i], entry, entrysize);
      i++;
    }
  }
  if (closedir(d)) return(-1);
  if (i == 0) return(-1);
  if (compar != NULL)
    qsort((void *)(*namelist), (size_t)i, sizeof(struct dirent *), compar);
    
  return(i);
}
#endif

static int
theora_write_frame(unsigned long w, unsigned long h, unsigned char *yuv, int last)
{
  th_ycbcr_buffer ycbcr;
  ogg_packet op;
  ogg_page og;

  unsigned long yuv_w;
  unsigned long yuv_h;

  unsigned char *yuv_y;
  unsigned char *yuv_u;
  unsigned char *yuv_v;

  unsigned int x;
  unsigned int y;

  /* Must hold: yuv_w >= w */
  yuv_w = (w + 15) & ~15;

  /* Must hold: yuv_h >= h */
  yuv_h = (h + 15) & ~15;

  ycbcr[0].width = yuv_w;
  ycbcr[0].height = yuv_h;
  ycbcr[0].stride = yuv_w;
  ycbcr[1].width = (chroma_format == TH_PF_444) ? yuv_w : (yuv_w >> 1);
  ycbcr[1].stride = ycbcr[1].width;
  ycbcr[1].height = (chroma_format == TH_PF_420) ? (yuv_h >> 1) : yuv_h;
  ycbcr[2].width = ycbcr[1].width;
  ycbcr[2].stride = ycbcr[1].stride;
  ycbcr[2].height = ycbcr[1].height;

  ycbcr[0].data = yuv_y = malloc(ycbcr[0].stride * ycbcr[0].height);
  ycbcr[1].data = yuv_u = malloc(ycbcr[1].stride * ycbcr[1].height);
  ycbcr[2].data = yuv_v = malloc(ycbcr[2].stride * ycbcr[2].height);

  for(y = 0; y < h; y++) {
    for(x = 0; x < w; x++) {
      yuv_y[x + y * yuv_w] = yuv[3 * (x + y * w) + 0];
    }
  }

  if (chroma_format == TH_PF_420) {
    for(y = 0; y < h; y += 2) {
      for(x = 0; x < w; x += 2) {
        yuv_u[(x >> 1) + (y >> 1) * (yuv_w >> 1)] =
          yuv[3 * (x + y * w) + 1];
        yuv_v[(x >> 1) + (y >> 1) * (yuv_w >> 1)] =
          yuv[3 * (x + y * w) + 2];
      }
    }
  } else if (chroma_format == TH_PF_444) {
    for(y = 0; y < h; y++) {
      for(x = 0; x < w; x++) {
        yuv_u[x + y * ycbcr[1].stride] = yuv[3 * (x + y * w) + 1];
        yuv_v[x + y * ycbcr[2].stride] = yuv[3 * (x + y * w) + 2];
      }
    }
  } else {  /* TH_PF_422 */
    for(y = 0; y < h; y += 1) {
      for(x = 0; x < w; x += 2) {
        yuv_u[(x >> 1) + y * ycbcr[1].stride] =
          yuv[3 * (x + y * w) + 1];
        yuv_v[(x >> 1) + y * ycbcr[2].stride] =
          yuv[3 * (x + y * w) + 2];
      }
    }    
  }

  /* Theora is a one-frame-in,one-frame-out system; submit a frame
     for compression and pull out the packet */
  /* in two-pass mode's second pass, we need to submit first-pass data */
  if(passno==2){
    int ret;
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

  if(th_encode_ycbcr_in(td, ycbcr)) {
    fprintf(stderr, "%s: error: could not encode frame\n",
      option_output);
    return 1;
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

  if(!th_encode_packetout(td, last, &op)) {
    fprintf(stderr, "%s: error: could not read packets\n",
      option_output);
    return 1;
  }

  if (passno!=1) { 
    ogg_stream_packetin(&ogg_os, &op);
    while(ogg_stream_pageout(&ogg_os, &og)) {
      fwrite(og.header, og.header_len, 1, ogg_fp);
      fwrite(og.body, og.body_len, 1, ogg_fp);
    }
  }  

  free(yuv_y);
  free(yuv_u);
  free(yuv_v);

  return 0;
}

static unsigned char
clamp(double d)
{
  if(d < 0)
    return 0;

  if(d > 255)
    return 255;

  return d;
}

static void
rgb_to_yuv(png_bytep *png,
           unsigned char *yuv,
           unsigned int w, unsigned int h)
{
  unsigned int x;
  unsigned int y;

  for(y = 0; y < h; y++) {
    for(x = 0; x < w; x++) {
      png_byte r;
      png_byte g;
      png_byte b;

      r = png[y][3 * x + 0];
      g = png[y][3 * x + 1];
      b = png[y][3 * x + 2];

      /* XXX: Cringe. */
      yuv[3 * (x + w * y) + 0] = clamp(
        0.299 * r
        + 0.587 * g
        + 0.114 * b);
      yuv[3 * (x + w * y) + 1] = clamp((0.436 * 255
        - 0.14713 * r
        - 0.28886 * g
        + 0.436 * b) / 0.872);
      yuv[3 * (x + w * y) + 2] = clamp((0.615 * 255
        + 0.615 * r
        - 0.51499 * g
        - 0.10001 * b) / 1.230);
    }
  }
}

static int
png_read(const char *pathname, unsigned int *w, unsigned int *h, unsigned char **yuv)
{
  FILE *fp;
  unsigned char header[8];
  png_structp png_ptr;
  png_infop info_ptr;
  png_infop end_ptr;
  png_bytep row_data;
  png_bytep *row_pointers;
  png_color_16p bkgd;
  png_uint_32 width;
  png_uint_32 height;
  int bit_depth;
  int color_type;
  int interlace_type;
  int compression_type;
  int filter_method;
  png_uint_32 y;

  fp = fopen(pathname, "rb");
  if(!fp) {
    fprintf(stderr, "%s: error: %s\n",
      pathname, strerror(errno));
    return 1;
  }

  fread(header, 1, 8, fp);
  if(png_sig_cmp(header, 0, 8)) {
    fprintf(stderr, "%s: error: %s\n",
      pathname, "not a PNG");
    fclose(fp);
    return 1;
  }

  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
    NULL, NULL, NULL);
  if(!png_ptr) {
    fprintf(stderr, "%s: error: %s\n",
      pathname, "couldn't create png read structure");
    fclose(fp);
    return 1;
  }

  info_ptr = png_create_info_struct(png_ptr);
  if(!info_ptr) {
    fprintf(stderr, "%s: error: %s\n",
      pathname, "couldn't create png info structure");
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    fclose(fp);
    return 1;
  }

  end_ptr = png_create_info_struct(png_ptr);
  if(!end_ptr) {
    fprintf(stderr, "%s: error: %s\n",
      pathname, "couldn't create png info structure");
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    return 1;
  }

  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);
  png_read_info(png_ptr, info_ptr);
  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
   &interlace_type, &compression_type, &filter_method);
  png_set_expand(png_ptr);
  if(bit_depth<8)png_set_packing(png_ptr);
  if(bit_depth==16)png_set_strip_16(png_ptr);
  if(!(color_type&PNG_COLOR_MASK_COLOR))png_set_gray_to_rgb(png_ptr);
  if(png_get_bKGD(png_ptr, info_ptr, &bkgd)){
    png_set_background(png_ptr, bkgd, PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);
  }
  /*Note that color_type 2 and 3 can also have alpha, despite not setting the
     PNG_COLOR_MASK_ALPHA bit.
    We always strip it to prevent libpng from overrunning our buffer.*/
  png_set_strip_alpha(png_ptr);

  row_data = (png_bytep)png_malloc(png_ptr,
    3*height*width*png_sizeof(*row_data));
  row_pointers = (png_bytep *)png_malloc(png_ptr,
    height*png_sizeof(*row_pointers));
  for(y = 0; y < height; y++) {
    row_pointers[y] = row_data + y*(3*width);
  }
  png_read_image(png_ptr, row_pointers);
  png_read_end(png_ptr, end_ptr);

  *w = width;
  *h = height;
  *yuv = malloc(*w * *h * 3);
  rgb_to_yuv(row_pointers, *yuv, *w, *h);

  png_free(png_ptr, row_pointers);
  png_free(png_ptr, row_data);
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);

  fclose(fp);
  return 0;
}

static int include_files (const struct dirent *de)
{
  char name[1024];
  int number = -1;
  sscanf(de->d_name, input_filter, &number);
  sprintf(name, input_filter, number);
  return !strcmp(name, de->d_name);
}

static int ilog(unsigned _v){
  int ret;
  for(ret=0;_v;ret++)_v>>=1;
  return ret;
}
      
int
main(int argc, char *argv[])
{
  int c,long_option_index;
  int i, n;
  char *input_mask;
  char *input_directory;
  char *scratch;
  th_comment       tc;
  struct dirent **png_files;
  int soft_target=0;
  int ret;
      
  while(1) {

    c=getopt_long(argc,argv,optstring,options,&long_option_index);
    if(c == EOF)
      break;

    switch(c) {
      case 'h':
        usage();
        break;
      case 'o':
        option_output = optarg;
        break;;
      case 'v':
        video_quality=rint(atof(optarg)*6.3);
        if(video_quality<0 || video_quality>63){
          fprintf(stderr,"Illegal video quality (choose 0 through 10)\n");
          exit(1);
        }
        video_rate=0;
        break;
      case 'V':
        video_rate=rint(atof(optarg)*1000);
        if(video_rate<1){
          fprintf(stderr,"Illegal video bitrate (choose > 0 please)\n");
          exit(1);
        }
        video_quality=0;
       break;
    case '\1':
      soft_target=1;
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
     case 's':
       video_aspect_numerator=rint(atof(optarg));
       break;
     case 'S':
       video_aspect_denominator=rint(atof(optarg));
       break;
     case 'f':
       video_fps_numerator=rint(atof(optarg));
       break;
     case 'F':
       video_fps_denominator=rint(atof(optarg));
       break;
     case '\5':
       chroma_format=TH_PF_444;
       break;
     case '\6':
       chroma_format=TH_PF_422;
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
        break;
      }
  }

  if(argc < 3) {
    usage();
  }

  if(soft_target){
    if(video_rate<=0){
      fprintf(stderr,"Soft rate target (--soft-target) requested without a bitrate (-V).\n");
      exit(1);
    }
    if(video_quality==-1)
      video_quality=0;
  }else{
    if(video_rate>0)
      video_quality=0;
    if(video_quality==-1)
      video_quality=48;
  }

  if(keyframe_frequency<=0){
    /*Use a default keyframe frequency of 64 for 1-pass (streaming) mode, and
       256 for two-pass mode.*/
    keyframe_frequency=twopass?256:64;
  }

  input_mask = argv[optind];
  if (!input_mask) {
    fprintf(stderr, "no input files specified; run with -h for help.\n");
    exit(1);
  }
  /* dirname and basename must operate on scratch strings */
  scratch = strdup(input_mask);
  input_directory = strdup(dirname(scratch));
  free(scratch);
  scratch = strdup(input_mask);
  input_filter = strdup(basename(scratch));
  free(scratch);

#ifdef DEBUG
  fprintf(stderr, "scanning %s with filter '%s'\n",
  input_directory, input_filter);
#endif
  n = scandir (input_directory, &png_files, include_files, alphasort);

  if (!n) {
    fprintf(stderr, "no input files found; run with -h for help.\n");
    exit(1);
  }

  ogg_fp = fopen(option_output, "wb");
  if(!ogg_fp) {
    fprintf(stderr, "%s: error: %s\n",
      option_output, "couldn't open output file");
    return 1;
  }

  srand(time(NULL));
  if(ogg_stream_init(&ogg_os, rand())) {
    fprintf(stderr, "%s: error: %s\n",
      option_output, "couldn't create ogg stream state");
    return 1;
  }

  for(passno=(twopass==3?1:twopass);passno<=(twopass==3?2:twopass);passno++){
    unsigned int w;
    unsigned int h;
    unsigned char *yuv;
    char input_png[1024];
    int last = 0;

    snprintf(input_png, 1023,"%s/%s", input_directory, png_files[0]->d_name);
    if(png_read(input_png, &w, &h, &yuv)) {
      fprintf(stderr, "could not read %s\n", input_png);
      exit(1);
    }

    if (passno!=2) fprintf(stderr,"%d frames, %dx%d\n",n,w,h);    

    /* setup complete.  Raw processing loop */
    switch(passno){
    case 0: case 2:
      fprintf(stderr,"\rCompressing....                                          \n");
      break;
    case 1:
      fprintf(stderr,"\rScanning first pass....                                  \n");
      break;
    }

    fprintf(stderr, "%s\n", input_png); 

    th_info_init(&ti);    
    ti.frame_width = ((w + 15) >>4)<<4;
    ti.frame_height = ((h + 15)>>4)<<4;
    ti.pic_width = w;
    ti.pic_height = h;
    ti.pic_x = 0;
    ti.pic_y = 0;
    ti.fps_numerator = video_fps_numerator;
    ti.fps_denominator = video_fps_denominator;
    ti.aspect_numerator = video_aspect_numerator;
    ti.aspect_denominator = video_aspect_denominator;
    ti.colorspace = TH_CS_UNSPECIFIED;
    ti.pixel_fmt = chroma_format;
    ti.target_bitrate = video_rate;
    ti.quality = video_quality;
    ti.keyframe_granule_shift=ilog(keyframe_frequency-1);

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
        if((keyframe_frequency*7>>1) > 5*video_fps_numerator/video_fps_denominator)
          arg=keyframe_frequency*7>>1;
        else
          arg=5*video_fps_numerator/video_fps_denominator;
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
        if(fseek(twopass_file,0,SEEK_SET)<0){
          fprintf(stderr,"Unable to seek in two-pass data file.\n");
          exit(1);
        }
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
    /* write the bitstream header packets with proper page interleave */
    th_comment_init(&tc);
    /* first packet will get its own page automatically */
    if(th_encode_flushheader(td,&tc,&op)<=0){
      fprintf(stderr,"Internal Theora library error.\n");
      exit(1); 
    }
    th_comment_clear(&tc);
    if(passno!=1){
      ogg_stream_packetin(&ogg_os,&op);
      if(ogg_stream_pageout(&ogg_os,&og)!=1){
        fprintf(stderr,"Internal Ogg library error.\n");
        exit(1);
      }
      fwrite(og.header,1,og.header_len,ogg_fp);
      fwrite(og.body,1,og.body_len,ogg_fp);
    }
    /* create the remaining theora headers */
    for(;;){
      ret=th_encode_flushheader(td,&tc,&op);
      if(ret<0){
        fprintf(stderr,"Internal Theora library error.\n");
        exit(1);
      }
      else if(!ret)break;
      if(passno!=1)ogg_stream_packetin(&ogg_os,&op);
    }
    /* Flush the rest of our headers. This ensures
       the actual data in each stream will start
       on a new page, as per spec. */
    if(passno!=1){
      for(;;){
        int result = ogg_stream_flush(&ogg_os,&og);
        if(result<0){
          /* can't get here */
          fprintf(stderr,"Internal Ogg library error.\n");
          exit(1);
        }
        if(result==0)break;
        fwrite(og.header,1,og.header_len,ogg_fp);
        fwrite(og.body,1,og.body_len,ogg_fp);
      }
    }

    i=0; last=0;
    do {
      if(i >= n-1) last = 1;
      if(theora_write_frame(w, h, yuv, last)) {
          fprintf(stderr,"Encoding error.\n");
        exit(1);
      }
      free(yuv);    
      i++;
      if (!last) {
        snprintf(input_png, 1023,"%s/%s", input_directory, png_files[i]->d_name);
        if(png_read(input_png, &w, &h, &yuv)) {
          fprintf(stderr, "could not read %s\n", input_png);
          exit(1);
        }
       fprintf(stderr, "%s\n", input_png);
      }      
    } while (!last);

    if(passno==1){
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
    th_encode_free(td);
  }

  if(ogg_stream_flush(&ogg_os, &og)) {
    fwrite(og.header, og.header_len, 1, ogg_fp);
    fwrite(og.body, og.body_len, 1, ogg_fp);
  }            

  free(input_directory);
  free(input_filter);

  while (n--) free(png_files[n]);
  free(png_files);  

  if(ogg_fp){
    fflush(ogg_fp);
    if(ogg_fp!=stdout)fclose(ogg_fp);
  }

  ogg_stream_clear(&ogg_os);
  if(twopass_file)fclose(twopass_file);
  fprintf(stderr,"\r   \ndone.\n\n");

  return 0;
}
