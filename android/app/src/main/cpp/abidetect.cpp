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

#include "cpu-features.h"
#include "abidetect.h"

static const char *className = "com/arthenica/mobileffmpeg/AbiDetect";

static JNINativeMethod methods[] = {
  {"getAbi", "()Ljava/lang/String;", (void*) Java_com_arthenica_mobileffmpeg_AbiDetect_getAbi}
};

jint JNI_OnLoad(JavaVM* vm, void*) {
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

    if (env->RegisterNatives(clazz, methods, 1) < 0) {
        LOGE("OnLoad failed to RegisterNatives for class %s", className);
        return JNI_FALSE;
    }

    return JNI_VERSION_1_6;
}

/*
 * Class:     com_arthenica_mobileffmpeg_AbiDetect
 * Method:    getAbi
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_arthenica_mobileffmpeg_AbiDetect_getAbi(JNIEnv* env, jclass) {
    AndroidCpuFamily family = android_getCpuFamily();

    if (family == ANDROID_CPU_FAMILY_ARM) {
        uint64_t features = android_getCpuFeatures();

        if (features & ANDROID_CPU_ARM_FEATURE_ARMv7) {
            if (features & ANDROID_CPU_ARM_FEATURE_NEON) {
                return env->NewStringUTF(ABI_ARMV7A_NEON);
            } else {
                return env->NewStringUTF(ABI_ARMV7A);
            }
        } else {
            return env->NewStringUTF(ABI_ARM);
        }

    } else if (family == ANDROID_CPU_FAMILY_ARM64) {
        return env->NewStringUTF(ABI_ARM64_V8A);
    } else if (family == ANDROID_CPU_FAMILY_X86) {
        return env->NewStringUTF(ABI_X86);
    } else if (family == ANDROID_CPU_FAMILY_X86_64) {
        return env->NewStringUTF(ABI_X86_64);
    } else {
        return env->NewStringUTF(ABI_UNKNOWN);
    }
}
