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

#ifndef MOBILE_FFMPEG_LOG_H
#define MOBILE_FFMPEG_LOG_H

#include <jni.h>
#include <android/log.h>
#include "libavutil/log.h"

/** Defines tag used for Android logging. */
#define LIB_NAME "mobile-ffmpeg"

/** Verbose Android logging macro. */
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LIB_NAME, __VA_ARGS__)

/** Debug Android logging macro. */
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LIB_NAME, __VA_ARGS__)

/** Info Android logging macro. */
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LIB_NAME, __VA_ARGS__)

/** Warn Android logging macro. */
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LIB_NAME, __VA_ARGS__)

/** Error Android logging macro. */
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LIB_NAME, __VA_ARGS__)

/*
 * Class:     com_arthenica_mobileffmpeg_Log
 * Method:    enableNativeRedirection
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_arthenica_mobileffmpeg_Log_enableNativeRedirection(JNIEnv *, jclass);

/*
 * Class:     com_arthenica_mobileffmpeg_Log
 * Method:    disableNativeRedirection
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_arthenica_mobileffmpeg_Log_disableNativeRedirection(JNIEnv *, jclass);

/*
 * Class:     com_arthenica_mobileffmpeg_Log
 * Method:    setNativeLevel
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_arthenica_mobileffmpeg_Log_setNativeLevel(JNIEnv *, jclass, jint);

/*
 * Class:     com_arthenica_mobileffmpeg_Log
 * Method:    getNativeLevel
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_arthenica_mobileffmpeg_Log_getNativeLevel(JNIEnv *, jclass);

#endif /* MOBILE_FFMPEG_LOG_H */
