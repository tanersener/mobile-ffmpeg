/* main.c
 * Command line interface.
 *
 * This fixed point version of shine is based on Gabriel Bouvigne's original
 * source, version 0.1.2
 * It was converted for use on Acorn computers running RISC OS and will require
 * the assembler multiply file to be replaced for other platforms.
 * 09/02/01 P.Everett
 *
 * Converted to a no-globals library-based system primarily for ARM-LINUX-GCC
 * but also works on x86 and x86_64 with the Makefile-generic, however
 * quality is worse right now on non-ARM as the noarch file uses bad math.
 * Jan 2, 2006 P.Roberts
 *
 */

/* Global headers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Required headers from libshine. */
#include "layer3.h"

/* Local header */
#include "main.h"
#include "wave.h"

/* Some global vars. */
char *infname, *outfname;
FILE *infile, *outfile;
int quiet = 0;
int _verbose = 0;
int stereo = STEREO;
int force_mono = 0;

int verbose()
{
  return _verbose;
}

/* Write out the MP3 file */
int write_mp3(long bytes, void *buffer, void *config)
{
  return fwrite(buffer, sizeof(unsigned char), bytes, outfile) / sizeof(unsigned char);
}

/* Output error message and exit */
void error(char *s)
{
  fprintf(stderr, "Error: %s\n", s);
  exit(1);
}

static void print_name()
{
  printf("shineenc (Liquidsoap version)\n");
}

static void print_usage()
{
  printf("Usage: shineenc [options] <infile> <outfile>\n\n");
  printf("Options:\n");
  printf(" -h            this help message\n");
  printf(" -b <bitrate>  set the bitrate [8-320], default 128kbit\n");
  printf(" -m            force encoder to operate in mono\n");
  printf(" -c            set copyright flag, default off\n");
  printf(" -j            encode in joint stereo (stereo data only)\n");
  printf(" -d            encode in dual-channel (stereo data only)\n");
  printf(" -q            quiet mode\n");
  printf(" -v            verbose mode\n");
}

/* Use these default settings, can be overridden */
static void set_defaults(shine_config_t *config)
{
  shine_set_config_mpeg_defaults(&config->mpeg);
}

/* Parse command line arguments */
static int parse_command(int argc, char** argv, shine_config_t *config)
{
  int i = 0;

  if(argc < 3) return 0;

  while (argv[++i][0] == '-' && argv[i][1] != '\000' && argv[i][1] != ' ')
    switch (argv[i][1]) {
      case 'b':
        config->mpeg.bitr = atoi(argv[++i]);
        break;

      case 'm':
        force_mono = 1;
        break;

      case 'j':
        stereo = JOINT_STEREO;
        break;

      case 'd':
        stereo = DUAL_CHANNEL;
        break;

      case 'c':
        config->mpeg.copyright = 1;
        break;

      case 'q':
        quiet = 1;
        _verbose = 0;
        break;

      case 'v':
        _verbose = 1;
        quiet = 0;
        break;

      case 'h':
      default :
        return 0;
    }

  if (argc - i != 2) return 0;
  infname = argv[i++];
  outfname = argv[i];
  return 1;
}

/* Print some info about what we're going to encode */
static void check_config(shine_config_t *config)
{
  static char *version_names[4] = { "2.5", "reserved", "II", "I" };
  static char *mode_names[4]    = { "stereo", "joint-stereo", "dual-channel", "mono" };
  static char *demp_names[4]    = { "none", "50/15us", "", "CITT" };

  printf("MPEG-%s layer III, %s  Psychoacoustic Model: Shine\n",
    version_names[shine_check_config(config->wave.samplerate, config->mpeg.bitr)],
    mode_names[config->mpeg.mode]);
  printf("Bitrate: %d kbps  ", config->mpeg.bitr);
  printf("De-emphasis: %s   %s %s\n",
    demp_names[config->mpeg.emph],
    ((config->mpeg.original) ? "Original" : ""),
    ((config->mpeg.copyright) ? "(C)" : ""));
  printf("Encoding \"%s\" to \"%s\"\n", infname, outfname);
}

int main(int argc, char **argv)
{
  wave_t         wave;
  time_t         start_time, end_time;
  int16_t        buffer[2*SHINE_MAX_SAMPLES];
  shine_config_t config;
  shine_t        s;
  int            written;
  unsigned char  *data;

  time(&start_time);

  /* Set the default MPEG encoding paramters - basically init the struct */
  set_defaults(&config);

  if (!parse_command(argc, argv, &config)) {
    print_usage();
    exit(1);
  }

  quiet = quiet || !strcmp(outfname, "-");

  if (!quiet) print_name();

  /* Open the input file and fill the config shine_wave_t header */
  if (!wave_open(infname, &wave, &config, quiet))
    error("Could not open WAVE file");

  infile = wave.file;

  if (force_mono)
    config.wave.channels = 1;

  /* See if samplerate and bitrate are valid */
  if (shine_check_config(config.wave.samplerate, config.mpeg.bitr) < 0)
    error("Unsupported samplerate/bitrate configuration.");

  /* open the output file */
  if (!strcmp(outfname, "-"))
    outfile = stdout;
  else
    outfile = fopen(outfname, "wb");
  if (!outfile) {
    fprintf(stderr, "Could not create \"%s\".\n", outfname);
    exit(1);
  }

  /* Set to stereo mode if wave data is stereo, mono otherwise. */
  if (config.wave.channels > 1)
    config.mpeg.mode = stereo;
  else
    config.mpeg.mode = MONO;

  /* Initiate encoder */
  s = shine_initialise(&config);

  // assert(s != NULL);

  /* Print some info about the file about to be created (optional) */
  if (!quiet) check_config(&config);

  int samples_per_pass = shine_samples_per_pass(s);

  /* All the magic happens here */
  while (wave_get(buffer, &wave, samples_per_pass)) {
    data = shine_encode_buffer_interleaved(s, buffer, &written);
    if ( write_mp3(written, data, &config) != written )
    {
       fprintf(stderr, "shineenc: write error\n");
       return 1;
    }
  }

  /* Flush and write remaining data. */
  data = shine_flush(s, &written);
  write_mp3(written, data, &config);

  /* Close encoder. */
  shine_close(s);

  /* Close the wave file (using the wav reader) */
  wave_close(&wave);

  /* Close the MP3 file */
  fclose(outfile);

  time(&end_time);
  end_time -= start_time;
  if (!quiet)
    printf("Finished in %02ld:%02ld:%02ld (%01.1fx realtime)\n", end_time / 3600, (end_time / 60) % 60, end_time % 60, (float)wave.duration / (float)end_time);

  return 0;
}
