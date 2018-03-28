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

#ifndef MOBILEFFMPEG_LOG_H
#define MOBILEFFMPEG_LOG_H

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <jni.h>
#include <android/log.h>

#define LIB_NAME "mobile-ffmpeg"

#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LIB_NAME, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LIB_NAME, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LIB_NAME, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LIB_NAME, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LIB_NAME, __VA_ARGS__)

/*
 * Class:     com_arthenica_mobileffmpeg_Log
 * Method:    startNativeCollector
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_arthenica_mobileffmpeg_Log_startNativeCollector(JNIEnv *, jobject);

/*
 * Class:     com_arthenica_mobileffmpeg_Log
 * Method:    stopNativeCollector
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_arthenica_mobileffmpeg_Log_stopNativeCollector(JNIEnv *, jobject);

#endif /* MOBILEFFMPEG_LOG_H */
