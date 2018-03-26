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

#ifndef MOBILEFFMPEG_H
#define MOBILEFFMPEG_H

#include <string>
#include <vector>
#include <jni.h>
#include <android/log.h>

#include "libavutil/ffversion.h"
#include "log.h"

#define MOBILE_FFMPEG_VERSION "1.0"

/*
 * Class:     com_arthenica_mobileffmpeg_FFmpeg
 * Method:    getFFmpegVersion
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_arthenica_mobileffmpeg_FFmpeg_getFFmpegVersion(JNIEnv *, jclass);

/*
 * Class:     com_arthenica_mobileffmpeg_FFmpeg
 * Method:    getVersion
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_arthenica_mobileffmpeg_FFmpeg_getVersion(JNIEnv *, jclass);

/*
 * Class:     com_arthenica_mobileffmpeg_FFmpeg
 * Method:    execute
 * Signature: ([Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_arthenica_mobileffmpeg_FFmpeg_execute(JNIEnv *, jclass, jobjectArray);

#endif /* MOBILEFFMPEG_H */
