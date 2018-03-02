#ifndef WAVE_H
#define WAVE_H

#include <stdio.h>

typedef struct {
        FILE *file;
        int  has_seek;
        int  channels;
        long length;
	long duration;
} wave_t;

unsigned char wave_open(const char *fname, wave_t *wave, shine_config_t *config, int quiet);
int  wave_get(int16_t *buffer, wave_t *wave, int samp_per_frame);
void wave_close(wave_t *wave);

#endif
