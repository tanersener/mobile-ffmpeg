/* ------------------------------------------------------------------
 * Copyright (C) 2009 Martin Storsjo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "wavwriter.h"
#include <interf_dec.h>

/* From WmfDecBytesPerFrame in dec_input_format_tab.cpp */
const int sizes[] = { 12, 13, 15, 17, 19, 20, 26, 31, 5, 6, 5, 5, 0, 0, 0, 0 };


int main(int argc, char *argv[]) {
	FILE* in;
	char header[6];
	int n;
	void *wav, *amr;
	if (argc < 3) {
		fprintf(stderr, "%s in.amr out.wav\n", argv[0]);
		return 1;
	}

	in = fopen(argv[1], "rb");
	if (!in) {
		perror(argv[1]);
		return 1;
	}
	n = fread(header, 1, 6, in);
	if (n != 6 || memcmp(header, "#!AMR\n", 6)) {
		fprintf(stderr, "Bad header\n");
		return 1;
	}

	wav = wav_write_open(argv[2], 8000, 16, 1);
	if (!wav) {
		fprintf(stderr, "Unable to open %s\n", argv[2]);
		return 1;
	}

	amr = Decoder_Interface_init();
	while (1) {
		uint8_t buffer[500], littleendian[320], *ptr;
		int size, i;
		int16_t outbuffer[160];
		/* Read the mode byte */
		n = fread(buffer, 1, 1, in);
		if (n <= 0)
			break;
		/* Find the packet size */
		size = sizes[(buffer[0] >> 3) & 0x0f];
		n = fread(buffer + 1, 1, size, in);
		if (n != size)
			break;

		/* Decode the packet */
		Decoder_Interface_Decode(amr, buffer, outbuffer, 0);

		/* Convert to little endian and write to wav */
		ptr = littleendian;
		for (i = 0; i < 160; i++) {
			*ptr++ = (outbuffer[i] >> 0) & 0xff;
			*ptr++ = (outbuffer[i] >> 8) & 0xff;
		}
		wav_write_data(wav, littleendian, 320);
	}
	fclose(in);
	Decoder_Interface_exit(amr);
	wav_write_close(wav);
	return 0;
}

