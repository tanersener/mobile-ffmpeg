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
#include <math.h>
#include <interf_enc.h>

int main(int argc, char *argv[]) {
	int i, j;
	void* amr;
	FILE* out;
	int sample_pos = 0;

	if (argc < 2) {
		fprintf(stderr, "%s out.amr\n", argv[0]);
		return 1;
	}

	amr = Encoder_Interface_init(0);
	out = fopen(argv[1], "wb");
	if (!out) {
		perror(argv[1]);
		return 1;
	}

	fwrite("#!AMR\n", 1, 6, out);
	for (i = 0; i < 1000; i++) {
		short buf[160];
		uint8_t outbuf[500];
		int n;
		for (j = 0; j < 160; j++) {
			buf[j] = 32767*sin(440*2*3.141592654*sample_pos/8000);
			sample_pos++;
		}
		n = Encoder_Interface_Encode(amr, MR475, buf, outbuf, 0);
		fwrite(outbuf, 1, n, out);
	}
	fclose(out);
	Encoder_Interface_exit(amr);

	return 0;
}

