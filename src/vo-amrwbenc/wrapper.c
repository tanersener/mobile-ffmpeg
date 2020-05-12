/* ------------------------------------------------------------------
 * Copyright (C) 2010 Martin Storsjo
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

#include "enc_if.h"
#include <stdlib.h>
#include <voAMRWB.h>
#include <cmnMemory.h>

struct encoder_state {
	VO_AUDIO_CODECAPI audioApi;
	VO_HANDLE handle;
	VO_MEM_OPERATOR memOperator;
	VO_CODEC_INIT_USERDATA userData;
};

void* E_IF_init(void) {
	struct encoder_state* state = (struct encoder_state*) malloc(sizeof(struct encoder_state));
	int frameType = VOAMRWB_RFC3267;
	voGetAMRWBEncAPI(&state->audioApi);
	state->memOperator.Alloc = cmnMemAlloc;
	state->memOperator.Copy = cmnMemCopy;
	state->memOperator.Free = cmnMemFree;
	state->memOperator.Set = cmnMemSet;
	state->memOperator.Check = cmnMemCheck;
	state->userData.memflag = VO_IMF_USERMEMOPERATOR;
	state->userData.memData = (VO_PTR)&state->memOperator;
	state->audioApi.Init(&state->handle, VO_AUDIO_CodingAMRWB, &state->userData);
	state->audioApi.SetParam(state->handle, VO_PID_AMRWB_FRAMETYPE, &frameType);
	return state;
}

void E_IF_exit(void* s) {
	struct encoder_state* state = (struct encoder_state*) s;
	state->audioApi.Uninit(state->handle);
	free(state);
}

int E_IF_encode(void* s, int mode, const short* speech, unsigned char* out, int dtx) {
	VO_CODECBUFFER inData, outData;
	VO_AUDIO_OUTPUTINFO outFormat;
	struct encoder_state* state = (struct encoder_state*) s;

	state->audioApi.SetParam(state->handle, VO_PID_AMRWB_MODE, &mode);
	state->audioApi.SetParam(state->handle, VO_PID_AMRWB_DTX, &dtx);
	inData.Buffer = (unsigned char*) speech;
	inData.Length = 640;
	outData.Buffer = out;
	state->audioApi.SetInputData(state->handle, &inData);
	state->audioApi.GetOutputData(state->handle, &outData, &outFormat);
	return outData.Length;
}

