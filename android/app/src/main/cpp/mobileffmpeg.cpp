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

#ifdef __cplusplus
extern "C" {
#endif

    // forward declaration for ffmpeg.c
    int execute(int argc, char **argv);

#ifdef __cplusplus
}
#endif

static const char *className = "com/arthenica/mobileffmpeg/FFmpeg";

static char *libName= "mobile-ffmpeg";

static JNINativeMethod methods[] = {
  {"getFFmpegVersion", "()Ljava/lang/String;", (void*) Java_com_arthenica_mobileffmpeg_FFmpeg_getFFmpegVersion},
  {"getVersion", "()Ljava/lang/String;", (void*) Java_com_arthenica_mobileffmpeg_FFmpeg_getVersion},
  {"execute", "([Ljava/lang/String;)I", (void*) Java_com_arthenica_mobileffmpeg_FFmpeg_execute},
};

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        LOGE("OnLoad failed to GetEnv for class %s.", className);
        return JNI_FALSE;
    }

    jclass clazz = env->FindClass(className);
    if (clazz == NULL) {
        LOGE("OnLoad failed to FindClass %s", className);
        return JNI_FALSE;
    }

    if (env->RegisterNatives(clazz, methods, 3) < 0) {
        LOGE("OnLoad failed to RegisterNatives for class %s", className);
        return JNI_FALSE;
    }

    return JNI_VERSION_1_6;
}

/*
 * Class:     com_arthenica_mobileffmpeg_FFmpeg
 * Method:    getFFmpegVersion
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_arthenica_mobileffmpeg_FFmpeg_getFFmpegVersion(JNIEnv* env, jclass object) {
    return env->NewStringUTF(FFMPEG_VERSION);
}

/*
 * Class:     com_arthenica_mobileffmpeg_FFmpeg
 * Method:    getVersion
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_arthenica_mobileffmpeg_FFmpeg_getVersion(JNIEnv* env, jclass object) {
    return env->NewStringUTF(MOBILE_FFMPEG_VERSION);
}

/*
 * Class:     com_arthenica_mobileffmpeg_FFmpeg
 * Method:    execute
 * Signature: ([Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_arthenica_mobileffmpeg_FFmpeg_execute(JNIEnv* env, jclass object, jobjectArray stringArray) {
    int stringCount = env->GetArrayLength(stringArray);

    // PREPARE
    char **argv = (char **)malloc(sizeof(char*) * (stringCount + 1));
    argv[0] = (char *) malloc(strlen(libName) + 1);
    strcpy(argv[0], libName);
    for (int i = 0; i < stringCount; i++) {
        jstring string = (jstring) (env->GetObjectArrayElement(stringArray, i));
        argv[i + 1] = (char*) env->GetStringUTFChars(string, 0);
        env->DeleteLocalRef(string);
    }

    // RUN
    int retCode = execute(stringCount + 1, argv);

    // CLEANUP
    for (int i = 0; i < stringCount; i++) {
        jstring string = (jstring) (env->GetObjectArrayElement(stringArray, i));
        env->ReleaseStringUTFChars(string, argv[i + 1]);
        env->DeleteLocalRef(string);
    }
    free(argv[0]);
    free(argv);

    return retCode;
}
