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
#include <interf_enc.h>
#include <unistd.h>
#include <stdlib.h>
#include "wavreader.h"

void usage(const char* name) {
	fprintf(stderr, "%s [-r bitrate] [-d] in.wav out.amr\n", name);
}

enum Mode findMode(const char* str) {
	struct {
		enum Mode mode;
		int rate;
	} modes[] = {
		{ MR475,  4750 },
		{ MR515,  5150 },
		{ MR59,   5900 },
		{ MR67,   6700 },
		{ MR74,   7400 },
		{ MR795,  7950 },
		{ MR102, 10200 },
		{ MR122, 12200 }
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
	enum Mode mode = MR122;
	int ch, dtx = 0;
	const char *infile, *outfile;
	FILE *out;
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
	if (sampleRate != 8000)
		fprintf(stderr, "Warning, AMR-NB uses 8000 Hz sample rate (WAV file has %d Hz)\n", sampleRate);
	inputSize = channels*2*160;
	inputBuf = (uint8_t*) malloc(inputSize);

	amr = Encoder_Interface_init(dtx);
	out = fopen(outfile, "wb");
	if (!out) {
		perror(outfile);
		return 1;
	}

	fwrite("#!AMR\n", 1, 6, out);
	while (1) {
		short buf[160];
		uint8_t outbuf[500];
		int read, i, n;
		read = wav_read_data(wav, inputBuf, inputSize);
		read /= channels;
		read /= 2;
		if (read < 160)
			break;
		for (i = 0; i < 160; i++) {
			const uint8_t* in = &inputBuf[2*channels*i];
			buf[i] = in[0] | (in[1] << 8);
		}
		n = Encoder_Interface_Encode(amr, mode, buf, outbuf, 0);
		fwrite(outbuf, 1, n, out);
	}
	free(inputBuf);
	fclose(out);
	Encoder_Interface_exit(amr);
	wav_read_close(wav);

	return 0;
}

