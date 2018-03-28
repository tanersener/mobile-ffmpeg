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

#include "log.h"

static JavaVM* globalVm;
static jclass logClass;
static jmethodID logMethod;

static int pipeFd[2];
static pthread_t logThread;
static bool logThreadEnabled = true;

const char *logClassName = "com/arthenica/mobileffmpeg/Log";
JNINativeMethod logMethods[] = {
    {"startNativeCollector", "()I", (void*) Java_com_arthenica_mobileffmpeg_Log_startNativeCollector},
    {"stopNativeCollector", "()I", (void*) Java_com_arthenica_mobileffmpeg_Log_stopNativeCollector}
};

jint JNI_OnLoad(JavaVM* vm, void*) {
    JNIEnv *jniEnv;
    if (vm->GetEnv(reinterpret_cast<void**>(&jniEnv), JNI_VERSION_1_6) != JNI_OK) {
        LOGE("OnLoad failed to GetEnv for class %s.", logClassName);
        return JNI_FALSE;
    }

    jniEnv->GetJavaVM(&globalVm);

    jclass clazz = jniEnv->FindClass(logClassName);
    if (clazz == NULL) {
        LOGE("OnLoad failed to FindClass %s.", logClassName);
        return JNI_FALSE;
    }

    logMethod = jniEnv->GetStaticMethodID(clazz, "log", "(Ljava/lang/String;)V");
    if (logMethod == NULL) {
        LOGE("OnLoad thread failed to GetMethodID for %s.", "log");
        globalVm->DetachCurrentThread();
        return JNI_FALSE;
    }

    if (jniEnv->RegisterNatives(clazz, logMethods, 2) < 0) {
        LOGE("OnLoad failed to RegisterNatives for class %s.", logClassName);
        return JNI_FALSE;
    }

    logClass = reinterpret_cast<jclass>(jniEnv->NewGlobalRef(clazz));

    return JNI_VERSION_1_6;
}

static void *logThreadFunction(void*) {
    ssize_t readSize;
    char buffer[128];

    JNIEnv *jniEnv;
    jint getEnvRc = globalVm->GetEnv(reinterpret_cast<void**>(&jniEnv), JNI_VERSION_1_6);
    if (getEnvRc != JNI_OK) {
        if (getEnvRc != JNI_EDETACHED) {
            LOGE("Log thread failed to GetEnv for class %s with rc %d.", logClassName, getEnvRc);
            return JNI_FALSE;
        }

        if (globalVm->AttachCurrentThread(&jniEnv, NULL) != 0) {
            LOGE("Log thread failed to AttachCurrentThread for class %s.", logClassName);
            return JNI_FALSE;
        }
    }

    LOGI("Native log thread started.");

    while(logThreadEnabled && ((readSize = read(pipeFd[0], buffer, sizeof(buffer) - 1)) > 0)) {
        if (readSize > 0) {
            if (buffer[readSize - 1] == '\n') {
                readSize--;
            }
            buffer[readSize] = 0;  /* add null-terminator */

            jstring message = (jstring) jniEnv->NewStringUTF(buffer);
            jniEnv->CallStaticVoidMethod(logClass, logMethod, message);
            jniEnv->DeleteLocalRef(message);
        }
    }

    globalVm->DetachCurrentThread();

    LOGI("Native log thread stopped.");

    return 0;
}

/*
 * Class:     com_arthenica_mobileffmpeg_Log
 * Method:    startNativeCollector
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_arthenica_mobileffmpeg_Log_startNativeCollector(JNIEnv *, jobject) {

    /* make stdout line-buffered and stderr unbuffered */
    setvbuf(stdout, 0, _IOLBF, 0);
    setvbuf(stderr, 0, _IONBF, 0);

    /* create the pipe and redirect stdout and stderr */
    pipe(pipeFd);
    dup2(pipeFd[1], 1);
    dup2(pipeFd[1], 2);

    /* spawn the logging thread */
    int rc = pthread_create(&logThread, 0, logThreadFunction, 0);
    if (rc != 0) {
        LOGE("Failed to create native log thread (rc=%d).", rc);
        return rc;
    }

    return 0;
}

/*
 * Class:     com_arthenica_mobileffmpeg_Log
 * Method:    stopNativeCollector
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_arthenica_mobileffmpeg_Log_stopNativeCollector(JNIEnv *, jobject) {
    logThreadEnabled = false;

    printf("Stopping native log thread\n");

    return 0;
}
