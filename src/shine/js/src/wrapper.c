#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "layer3.h"

shine_t shine_js_init(int channels, int samplerate, int mode, int bitr) {
  shine_config_t config;
  config.wave.channels = channels;
  config.wave.samplerate = samplerate;

  shine_set_config_mpeg_defaults(&config.mpeg);
  config.mpeg.mode = mode;
  config.mpeg.bitr = bitr;

  return shine_initialise(&config);
} 
