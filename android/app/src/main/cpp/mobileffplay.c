/*
 * Copyright (c) 2020 Taner Sener
 *
 * This file is part of MobileFFmpeg.
 *
 * MobileFFmpeg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MobileFFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with MobileFFmpeg.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <jni.h>
#include "libavcodec/jni.h"

/** Forward declaration for function defined in fftools_ffplay.c */
int ffplay_execute(int argc, char **argv);

/** Forward declaration for functions defined in SDl_android.c */
void set_mobile_ffmpeg_ffplay_execute(int (*ffplay_execute_function)(int argc, char **argv));
jint SDL_Android_Initialize(JavaVM* vm, void* reserved);

/**
 * Initializes SDL for FFplay. It must be called before other SDL functions.
 */
JNIEXPORT void JNICALL Java_com_arthenica_mobileffmpeg_FFplay_nativeSDLInit(JNIEnv *env, jclass object) {
    set_mobile_ffmpeg_ffplay_execute(ffplay_execute);
    JavaVM *globalVm = av_jni_get_java_vm(NULL);
    if (globalVm) {
        SDL_Android_Initialize(globalVm, NULL);
    }
}
