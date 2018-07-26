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

#include "mobileffmpeg.h"

/** Forward declaration for function defined in ffmpeg.c */
int execute(int argc, char **argv);

/** Full name of the Java class that owns native functions in this file. */
const char *ffmpegClassName = "com/arthenica/mobileffmpeg/FFmpeg";

/** Prototypes of native functions defined by this file. */
JNINativeMethod ffmpegMethods[] = {
  {"getFFmpegVersion", "()Ljava/lang/String;", (void*) Java_com_arthenica_mobileffmpeg_FFmpeg_getFFmpegVersion},
  {"getVersion", "()Ljava/lang/String;", (void*) Java_com_arthenica_mobileffmpeg_FFmpeg_getVersion},
  {"execute", "([Ljava/lang/String;)I", (void*) Java_com_arthenica_mobileffmpeg_FFmpeg_execute}
};

/**
 * Called when 'mobileffmpeg' native library is loaded.
 *
 * \param vm pointer to the running virtual machine
 * \param reserved reserved
 * \return JNI version needed by 'mobileffmpeg' library
 */
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if ((*vm)->GetEnv(vm, (void**)(&env), JNI_VERSION_1_6) != JNI_OK) {
        LOGE("OnLoad failed to GetEnv for class %s.\n", ffmpegClassName);
        return JNI_FALSE;
    }

    jclass ffmpegClass = (*env)->FindClass(env, ffmpegClassName);
    if (ffmpegClass == NULL) {
        LOGE("OnLoad failed to FindClass %s.\n", ffmpegClassName);
        return JNI_FALSE;
    }

    if ((*env)->RegisterNatives(env, ffmpegClass, ffmpegMethods, 3) < 0) {
        LOGE("OnLoad failed to RegisterNatives for class %s.\n", ffmpegClassName);
        return JNI_FALSE;
    }

    return JNI_VERSION_1_6;
}

/**
 * Returns FFmpeg version bundled within the library.
 *
 * \param env pointer to native method interface
 * \param object reference to the class on which this method is invoked
 * \return FFmpeg version string
 */
JNIEXPORT jstring JNICALL Java_com_arthenica_mobileffmpeg_FFmpeg_getFFmpegVersion(JNIEnv *env, jclass object) {
    return (*env)->NewStringUTF(env, FFMPEG_VERSION);
}

/**
 * Returns MobileFFmpeg library version.
 *
 * \param env pointer to native method interface
 * \param object reference to the class on which this method is invoked
 * \return MobileFFmpeg version string
 */
JNIEXPORT jstring JNICALL Java_com_arthenica_mobileffmpeg_FFmpeg_getVersion(JNIEnv *env, jclass object) {
    return (*env)->NewStringUTF(env, MOBILE_FFMPEG_VERSION);
}

/**
 * Synchronously executes FFmpeg command with arguments provided.
 *
 * \param env pointer to native method interface
 * \param object reference to the class on which this method is invoked
 * \param stringArray reference to the object holding FFmpeg command arguments
 * \return zero on successful execution, non-zero on error
 */
JNIEXPORT jint JNICALL Java_com_arthenica_mobileffmpeg_FFmpeg_execute(JNIEnv *env, jclass object, jobjectArray stringArray) {
    jstring *tempArray = NULL;
    int argumentCount = 1;
    char **argv = NULL;

    if (stringArray != NULL) {
        int programArgumentCount = (*env)->GetArrayLength(env, stringArray);
        argumentCount = programArgumentCount + 1;

        tempArray = (jstring *) malloc(sizeof(jstring) * programArgumentCount);
    }

    /* PRESERVE USAGE FORMAT
     *
     * ffmpeg <arguments>
     */
    argv = (char **)malloc(sizeof(char*) * (argumentCount));
    argv[0] = (char *)malloc(sizeof(char) * (strlen(LIB_NAME) + 1));
    strcpy(argv[0], LIB_NAME);

    // PREPARE
    if (stringArray != NULL) {
        for (int i = 0; i < (argumentCount - 1); i++) {
            tempArray[i] = (jstring) (*env)->GetObjectArrayElement(env, stringArray, i);
            argv[i + 1] = (char *) (*env)->GetStringUTFChars(env, tempArray[i], 0);
        }
    }

    // RUN
    int retCode = execute(argumentCount, argv);

    // CLEANUP
    if (tempArray != NULL) {
        for (int i = 0; i < (argumentCount - 1); i++) {
            (*env)->ReleaseStringUTFChars(env, tempArray[i], argv[i + 1]);
        }

        free(tempArray);
    }
    free(argv[0]);
    free(argv);

    return retCode;
}
