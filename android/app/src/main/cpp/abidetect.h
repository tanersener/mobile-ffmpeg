/*
 * Copyright (c) 2018 Taner Sener
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

#ifndef MOBILEFFMPEG_ABIDETECT_H
#define MOBILEFFMPEG_ABIDETECT_H

#include <jni.h>
#include "log.h"

#define ABI_ARMV7A_NEON "armeabi-v7a-neon"
#define ABI_ARMV7A "armeabi-v7a"
#define ABI_ARM "armeabi"
#define ABI_X86 "x86"
#define ABI_X86_64 "x86_64"
#define ABI_ARM64_V8A "arm64-v8a"
#define ABI_UNKNOWN "unknown"

/*
 * Class:     com_arthenica_mobileffmpeg_AbiDetect
 * Method:    getAbi
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_arthenica_mobileffmpeg_AbiDetect_getAbi(JNIEnv *, jclass);

#endif /* MOBILEFFMPEG_ABIDETECT_H */
