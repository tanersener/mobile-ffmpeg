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

/** Global reference to the virtual machine running */
static JavaVM *globalVm;

/** Global reference of Log class in Java side */
static jclass logClass;

/** Global reference of forward log method in Java side */
static jmethodID logMethod;

/** file descriptor of the pipe created */
static int pipeFd[2];

/** log collector thread variable */
static pthread_t logThread;

/** Holds whether log collector thread is enabled or not */
static int logThreadEnabled = 1;

/** Full name of the Java class that owns native functions in this file. */
const char *logClassName = "com/arthenica/mobileffmpeg/Log";

/** Prototypes of native functions defined by this file. */
JNINativeMethod logMethods[] = {
    {"startNativeCollector", "()I", (void*) Java_com_arthenica_mobileffmpeg_Log_startNativeCollector},
    {"stopNativeCollector", "()I", (void*) Java_com_arthenica_mobileffmpeg_Log_stopNativeCollector}
};

/**
 * Called when 'ffmpeglog' native library is loaded.
 *
 * \param vm pointer to the running virtual machine
 * \param reserved reserved
 * \return JNI version needed by 'ffmpeglog' library
 */
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_6) != JNI_OK) {
        LOGE("OnLoad failed to GetEnv for class %s.", logClassName);
        return JNI_FALSE;
    }

    (*env)->GetJavaVM(env, &globalVm);

    jclass clazz = (*env)->FindClass(env, logClassName);
    if (clazz == NULL) {
        LOGE("OnLoad failed to FindClass %s.", logClassName);
        return JNI_FALSE;
    }

    logMethod = (*env)->GetStaticMethodID(env, clazz, "log", "([B)V");
    if (logMethod == NULL) {
        LOGE("OnLoad thread failed to GetMethodID for %s.", "log");
        (*globalVm)->DetachCurrentThread(globalVm);
        return JNI_FALSE;
    }

    if ((*env)->RegisterNatives(env, clazz, logMethods, 2) < 0) {
        LOGE("OnLoad failed to RegisterNatives for class %s.", logClassName);
        return JNI_FALSE;
    }

    logClass = (jclass) ((*env)->NewGlobalRef(env, clazz));

    return JNI_VERSION_1_6;
}

/**
 * Native log collector main thread function.
 */
static void *logThreadFunction() {
    int readSize;
    char buffer[512];

    JNIEnv *env;
    jint getEnvRc = (*globalVm)->GetEnv(globalVm, (void**) &env, JNI_VERSION_1_6);
    if (getEnvRc != JNI_OK) {
        if (getEnvRc != JNI_EDETACHED) {
            LOGE("Log thread failed to GetEnv for class %s with rc %d.", logClassName, getEnvRc);
            return JNI_FALSE;
        }

        if ((*globalVm)->AttachCurrentThread(globalVm, &env, NULL) != 0) {
            LOGE("Log thread failed to AttachCurrentThread for class %s.", logClassName);
            return JNI_FALSE;
        }
    }

    LOGI("Native log thread started.");

    while(logThreadEnabled && ((readSize = (int)read(pipeFd[0], buffer, sizeof(buffer) - 1)) > 0)) {
        if (readSize > 0) {
            if (buffer[readSize - 1] == '\n') {
                readSize--;
            }
            buffer[readSize] = 0;  /* add null-terminator */

            jbyteArray byteArray = (jbyteArray) (*env)->NewByteArray(env, readSize);
            (*env)->SetByteArrayRegion(env, byteArray, 0, readSize, (jbyte *)buffer);
            (*env)->CallStaticVoidMethod(env, logClass, logMethod, byteArray);
            (*env)->DeleteLocalRef(env, byteArray);
        }
    }

    (*globalVm)->DetachCurrentThread(globalVm);

    LOGI("Native log thread stopped.");

    return 0;
}

/**
 * Starts native log collector. Native log collector creates a pipe and redirects stdout and stderr to it. Then starts
 * a thread, reads data written to this pipe and forwards it to the Java side.
 *
 * \param env pointer to native method interface
 * \param object reference to the object on which this method is invoked
 * \return zero on success, non-zero if an error occurs
 */
JNIEXPORT jint JNICALL Java_com_arthenica_mobileffmpeg_Log_startNativeCollector(JNIEnv *env, jobject object) {

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

/**
 * Initiates a stop request for native log collector thread. Please note that when this function returns collector
 * thread might be still alive.
 *
 * \param env pointer to native method interface
 * \param object reference to the object on which this method is invoked
 * \return zero on success, non-zero if an error occurs
 */
JNIEXPORT jint JNICALL Java_com_arthenica_mobileffmpeg_Log_stopNativeCollector(JNIEnv *env, jobject object) {
    logThreadEnabled = 0;

    LOGI("Stopping native log thread\n");

    return 0;
}
