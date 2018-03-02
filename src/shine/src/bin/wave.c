/* wave.c
 *
 * MS Wave files store data 'little endian' sytle. These functions will only
 * work on 'little endian' machines. 09/02/01 P.Everett
 * note. both Acorn/RISC OS and PC/DOS are little endian.
 */

/* Required headers from libshine. */
#include "layer3.h"

/* Local header */
#include <stdint.h>
#include <string.h>
#include "main.h"
#include "wave.h"

/* RISC OS specifics */
#define WAVE  0xfb1      /* Wave filetype */
#define DATA  0xffd      /* Data filetype */

#define MODE_MONO 3

typedef struct {
  char id[4];
  uint32_t length;
} riff_chunk_header_t;

typedef struct {
  riff_chunk_header_t header;
  char wave[4];
} wave_chunk_t;

typedef struct {
  riff_chunk_header_t header;
  uint16_t format;       /* MS PCM = 1 */
  uint16_t channels;     /* channels, mono = 1, stereo = 2 */
  uint32_t sample_rate;  /* samples per second = 44100 */
  uint32_t byte_rate;    /* bytes per second = samp_rate * byte_samp = 176400 */
  uint16_t frame_size;   /* block align (bytes per sample) = channels * bits_per_sample / 8 = 4 */
  uint16_t depth;        /* bits per sample = 16 for MS PCM (format specific) */
} fmt_chunk_t;

void wave_seek(FILE *file, int has_seek, uint32_t bytes) {
  uint32_t i;
  if (has_seek == 1)
    fseek(file, bytes, SEEK_CUR);
  else {
    for (i = 0; i < bytes; i++)
      getc(file);
  }
}

unsigned char wave_get_chunk_header(FILE *file, int has_seek, const char id[4], riff_chunk_header_t *header)
{
  unsigned char found = 0;
  uint32_t chunk_length;

  if (verbose())
    fprintf(stderr, "Looking for chunk '%s'\n", id);

  while(!found) {
    if (fread(header, sizeof(riff_chunk_header_t), 1, file) != 1) {
      if (feof(file))
        return 0;
      else
        error("Read error");
    }

    /* chunks must be word-aligned, chunk data doesn't need to */
    chunk_length = header->length + header->length % 2;
    if (verbose()) {
      fprintf(stderr, "Found chunk '%.4s', length: %u\n", header->id, header->length);
    }

    if (strncmp(header->id, id, 4) == 0)
      return 1;
    
    wave_seek(file, has_seek, chunk_length);
  }

  return 1;
}


void wave_close(wave_t *wave)
{
  fclose(wave->file);
}


/*
 * wave_open:
 * ----------
 * Opens and verifies the header of the Input Wave file. The file pointer is
 * left pointing to the start of the samples.
 */
unsigned char wave_open(const char *fname, wave_t *wave, shine_config_t *config, int quiet)
{
  static char *channel_mappings[] = { NULL, "mono", "stereo" };
  wave_chunk_t wave_chunk;
  fmt_chunk_t fmt_chunk;
  riff_chunk_header_t data_chunk;
  uint32_t fmt_data, fmt_length;

  if (!strcmp(fname, "-")) {
     /* TODO: support raw PCM stream with commandline parameters specifying format */
    wave->file = stdin;
    wave->has_seek = 0;
  } else {
    wave->file = fopen(fname, "rb");
    wave->has_seek = 1;
  }

  if (!wave->file)
    error("Unable to open file");

  if (fread(&wave_chunk, sizeof(wave_chunk), 1, wave->file) != 1)
    error("Invalid header");

  if (strncmp(wave_chunk.header.id, "RIFF", 4) != 0)
    error("Not a MS-RIFF file");

  if (strncmp(wave_chunk.wave, "WAVE", 4) != 0)
    error("Not a WAVE audio file");

  /* Check the fmt chunk */
  if (!wave_get_chunk_header(wave->file, wave->has_seek, "fmt ", (riff_chunk_header_t *)&fmt_chunk))
    error("WAVE fmt chunk not found");

  fmt_data = sizeof(fmt_chunk_t) - sizeof(riff_chunk_header_t);

  if(fread(&fmt_chunk.format, fmt_data, 1, wave->file) != 1)
    error("Read error");

  if (verbose())
    fprintf(stderr, "WAVE format: %u\n", fmt_chunk.format);

  if (fmt_chunk.format != 1)
    error("Unknown WAVE format");

  if (fmt_chunk.channels > 2)
    error("More than 2 channels");

  if (fmt_chunk.depth != 16)
    error("Unsupported PCM bit depth");

  // Skip remaining data.
  fmt_length = fmt_chunk.header.length + fmt_chunk.header.length % 2;
  if (fmt_length > fmt_data)
    wave_seek(wave->file, wave->has_seek, fmt_length - fmt_data);

  /* Position the file pointer at the data chunk */
  if (!wave_get_chunk_header(wave->file, wave->has_seek, "data", &data_chunk))
    error("WAVE data chunk not found");

  config->wave.channels   = fmt_chunk.channels;
  config->wave.samplerate = fmt_chunk.sample_rate;

  wave->channels = fmt_chunk.channels;
  wave->length   = data_chunk.length;
  wave->duration = data_chunk.length / fmt_chunk.byte_rate;

  if (!quiet)
    printf("%s, %s %ldHz %ldbit, duration: %02ld:%02ld:%02ld\n",
      "WAVE PCM Data", channel_mappings[fmt_chunk.channels], (long)fmt_chunk.sample_rate, (long)fmt_chunk.depth,
      (long)wave->duration / 3600, (long)(wave->duration / 60) % 60, (long)wave->duration % 60);
  return 1;
}

#ifdef SHINE_BIG_ENDIAN
#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))
#define bswap_16(x) __builtin_bswap16(x)
#else
#define bswap_16(x) ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8))
#endif

void swap_buffer(int16_t *sample_buffer, int length)
{
  int16_t *end = sample_buffer + length;

  if (length >= 2 * sizeof(long) / sizeof(int16_t)) {
    const unsigned long mask = (~0UL / 0xffff) * 0xff;  /* 0x00ff00ff or 0x00ff00ff00ff00ff */
    unsigned long *long_ptr = (unsigned long *)((unsigned long)sample_buffer & -sizeof(long));

    /* make sample_buffer aligned on word boundary */
    if ((int16_t *)long_ptr != sample_buffer) {
      long_ptr++;
      do {
        register uint16_t tmp = *sample_buffer++;
        sample_buffer[-1] = bswap_16(tmp);
      } while (sample_buffer != (int16_t *)long_ptr);
    }

    while ((int16_t *)(long_ptr + 1) <= end) {
      register unsigned long tmp = *long_ptr++;
      long_ptr[-1] = ((tmp & mask) << 8) | ((tmp >> 8) & mask);
    }

    sample_buffer = (int16_t *)long_ptr;
  }

  while (sample_buffer < end) {
    register uint16_t tmp = *sample_buffer++;
    sample_buffer[-1] = bswap_16(tmp);
  }
}
#endif

/*
 * read_samples:
 * -------------
 */

/* TODO: respect data chunk length */
int read_samples(int16_t *sample_buffer, int frame_size, FILE *file)
{
  int samples_read=0;

  samples_read = fread(sample_buffer,sizeof(int16_t),frame_size, file);

#ifdef SHINE_BIG_ENDIAN
  swap_buffer(sample_buffer, samples_read);
#endif

  if(samples_read<frame_size && samples_read>0) { /* Pad sample with zero's */
    memset(sample_buffer + samples_read, 0, (frame_size - samples_read) * sizeof(int16_t));
    samples_read = frame_size;
  }

  return samples_read;
}

/*
 * wave_get:
 * ---------
 * Expects an interleaved 16bit pcm stream from read_samples, which it
 * reads into the given buffer.
 */
int wave_get(int16_t *buffer, wave_t *wave, int samp_per_pass)
{
  return read_samples(buffer,samp_per_pass*wave->channels, wave->file);
}

