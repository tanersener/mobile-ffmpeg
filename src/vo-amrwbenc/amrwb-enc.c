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
#include <enc_if.h>
#include <unistd.h>
#include <stdlib.h>
#include "wavreader.h"

void usage(const char* name) {
	fprintf(stderr, "%s [-r bitrate] [-d] in.wav out.amr\n", name);
}

int findMode(const char* str) {
	struct {
		int mode;
		int rate;
	} modes[] = {
		{ 0,  6600 },
		{ 1,  8850 },
		{ 2, 12650 },
		{ 3, 14250 },
		{ 4, 15850 },
		{ 5, 18250 },
		{ 6, 19850 },
		{ 7, 23050 },
		{ 8, 23850 }
	};
	int rate = atoi(str);
	int closest = -1;
	int closestdiff = 0;
	unsigned int i;
	for (i = 0; i < sizeof(modes)/sizeof(modes[0]); i++) {
		if (modes[i].rate == rate)
			return modes[i].mode;
		if (closest < 0 || closestdiff > abs(modes[i].rate - rate)) {
			closest = i;
			closestdiff = abs(modes[i].rate - rate);
		}
	}
	fprintf(stderr, "Using bitrate %d\n", modes[closest].rate);
	return modes[closest].mode;
}

int main(int argc, char *argv[]) {
	int mode = 8;
	int ch, dtx = 0;
	const char *infile, *outfile;
	FILE* out;
	void *wav, *amr;
	int format, sampleRate, channels, bitsPerSample;
	int inputSize;
	uint8_t* inputBuf;
	while ((ch = getopt(argc, argv, "r:d")) != -1) {
		switch (ch) {
		case 'r':
			mode = findMode(optarg);
			break;
		case 'd':
			dtx = 1;
			break;
		case '?':
		default:
			usage(argv[0]);
			return 1;
		}
	}
	if (argc - optind < 2) {
		usage(argv[0]);
		return 1;
	}
	infile = argv[optind];
	outfile = argv[optind + 1];


	wav = wav_read_open(infile);
	if (!wav) {
		fprintf(stderr, "Unable to open wav file %s\n", infile);
		return 1;
	}
	if (!wav_get_header(wav, &format, &channels, &sampleRate, &bitsPerSample, NULL)) {
		fprintf(stderr, "Bad wav file %s\n", infile);
		return 1;
	}
	if (format != 1) {
		fprintf(stderr, "Unsupported WAV format %d\n", format);
		return 1;
	}
	if (bitsPerSample != 16) {
		fprintf(stderr, "Unsupported WAV sample depth %d\n", bitsPerSample);
		return 1;
	}
	if (channels != 1)
		fprintf(stderr, "Warning, only compressing one audio channel\n");
	if (sampleRate != 16000)
		fprintf(stderr, "Warning, AMR-WB uses 16000 Hz sample rate (WAV file has %d Hz)\n", sampleRate);
	inputSize = channels*2*320;
	inputBuf = (uint8_t*) malloc(inputSize);

	amr = E_IF_init();
	out = fopen(outfile, "wb");
	if (!out) {
		perror(outfile);
		return 1;
	}

	fwrite("#!AMR-WB\n", 1, 9, out);
	while (1) {
		int read, i, n;
		short buf[320];
		uint8_t outbuf[500];

		read = wav_read_data(wav, inputBuf, inputSize);
		read /= channels;
		read /= 2;
		if (read < 320)
			break;
		for (i = 0; i < 320; i++) {
			const uint8_t* in = &inputBuf[2*channels*i];
			buf[i] = in[0] | (in[1] << 8);
		}
		n = E_IF_encode(amr, mode, buf, outbuf, dtx);
		fwrite(outbuf, 1, n, out);
	}
	free(inputBuf);
	fclose(out);
	E_IF_exit(amr);
	wav_read_close(wav);

	return 0;
}

