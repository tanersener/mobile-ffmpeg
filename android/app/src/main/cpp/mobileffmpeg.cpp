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

static const char *className = "com/arthenica/mobileffmpeg/FFmpeg";

static JNINativeMethod methods[] = {
  {"getFFmpegVersion", "()Ljava/lang/String;", (void*) Java_com_arthenica_mobileffmpeg_FFmpeg_getFFmpegVersion},
  {"getVersion", "()Ljava/lang/String;", (void*) Java_com_arthenica_mobileffmpeg_FFmpeg_getVersion},
  {"getABI", "()Ljava/lang/String;", (void*) Java_com_arthenica_mobileffmpeg_FFmpeg_getABI},
  {"execute", "([Ljava/lang/String;)Ljava/lang/String;", (void*) Java_com_arthenica_mobileffmpeg_FFmpeg_execute},
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

    if (env->RegisterNatives(clazz, methods, 4) < 0) {
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
 * Method:    getABI
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_arthenica_mobileffmpeg_FFmpeg_getABI(JNIEnv* env, jclass object) {
    #if defined(__arm__)
        #if defined(__ARM_ARCH_7A__)
            #if defined(__ARM_NEON__)
                return env->NewStringUTF(ABI_ARMV7A_NEON);
            #else
                return env->NewStringUTF(ABI_ARMV7A);
            #endif
        #else
            return env->NewStringUTF(ABI_ARM);
        #endif
    #elif defined(__i386__)
        return env->NewStringUTF(ABI_X86);
    #elif defined(__x86_64__)
        return env->NewStringUTF(ABI_X86_64);
    #elif defined(__aarch64__)
        return env->NewStringUTF(ABI_ARM64_V8A);
    #else
        return env->NewStringUTF(ABI_UNKNOWN);
    #endif
}

/*
 * Class:     com_arthenica_mobileffmpeg_FFmpeg
 * Method:    execute
 * Signature: ([Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_arthenica_mobileffmpeg_FFmpeg_execute(JNIEnv* env, jclass object, jobjectArray stringArray) {
    int stringCount = env->GetArrayLength(stringArray);
    std::string argv("");

    for (int i=0; i<stringCount; i++) {
        jstring string = (jstring) (env->GetObjectArrayElement(stringArray, i));
        const char* rawString = env->GetStringUTFChars(string, 0);

        if (i != 0) {
            argv.append(" ");
        }
        argv.append(rawString);

        env->ReleaseStringUTFChars(string, rawString);
        env->DeleteLocalRef(string);
    }

    return env->NewStringUTF(argv.c_str());
}
