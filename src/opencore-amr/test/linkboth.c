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
#include <interf_dec.h>
#include <interf_enc.h>
#include <dec_if.h>

int main(int argc, char *argv[]) {
#ifndef DISABLE_AMRNB_DECODER
	void* amrnb = Decoder_Interface_init();
	Decoder_Interface_exit(amrnb);
#endif
#ifndef DISABLE_AMRNB_ENCODER
	void* amrnb_enc = Encoder_Interface_init(0);
	Encoder_Interface_exit(amrnb_enc);
#endif
	void* amrwb = D_IF_init();
	D_IF_exit(amrwb);
	return 0;
}

