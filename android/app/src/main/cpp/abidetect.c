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

/** Full name of the Java class that owns native functions in this file. */
const char *abiDetectClassName = "com/arthenica/mobileffmpeg/AbiDetect";

/** Prototypes of native functions defined by this file. */
JNINativeMethod abiDetectMethods[] = {
  {"getAbi", "()Ljava/lang/String;", (void*) Java_com_arthenica_mobileffmpeg_AbiDetect_getAbi}
};

/**
 * Called when 'abidetect' native library is loaded.
 *
 * \param vm pointer to the running virtual machine
 * \param reserved reserved
 * \return JNI version needed by 'abidetect' library
 */
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_6) != JNI_OK) {
        LOGE("OnLoad failed to GetEnv for class %s.\n", abiDetectClassName);
        return JNI_FALSE;
    }

    jclass abiDetectClass = (*env)->FindClass(env, abiDetectClassName);
    if (abiDetectClass == NULL) {
        LOGE("OnLoad failed to FindClass %s.\n", abiDetectClassName);
        return JNI_FALSE;
    }

    if ((*env)->RegisterNatives(env, abiDetectClass, abiDetectMethods, 1) < 0) {
        LOGE("OnLoad failed to RegisterNatives for class %s.\n", abiDetectClassName);
        return JNI_FALSE;
    }

    return JNI_VERSION_1_6;
}

/**
 * Returns running ABI name.
 *
 * \param env pointer to native method interface
 * \param object reference to the class on which this method is invoked
 * \return running ABI name as UTF string
 */
JNIEXPORT jstring JNICALL Java_com_arthenica_mobileffmpeg_AbiDetect_getAbi(JNIEnv *env, jclass object) {
    AndroidCpuFamily family = android_getCpuFamily();

    if (family == ANDROID_CPU_FAMILY_ARM) {
        uint64_t features = android_getCpuFeatures();

        if (features & ANDROID_CPU_ARM_FEATURE_ARMv7) {
            if (features & ANDROID_CPU_ARM_FEATURE_NEON) {
                return (*env)->NewStringUTF(env, ABI_ARMV7A_NEON);
            } else {
                return (*env)->NewStringUTF(env, ABI_ARMV7A);
            }
        } else {
            return (*env)->NewStringUTF(env, ABI_ARM);
        }

    } else if (family == ANDROID_CPU_FAMILY_ARM64) {
        return (*env)->NewStringUTF(env, ABI_ARM64_V8A);
    } else if (family == ANDROID_CPU_FAMILY_X86) {
        return (*env)->NewStringUTF(env, ABI_X86);
    } else if (family == ANDROID_CPU_FAMILY_X86_64) {
        return (*env)->NewStringUTF(env, ABI_X86_64);
    } else {
        return (*env)->NewStringUTF(env, ABI_UNKNOWN);
    }
}
