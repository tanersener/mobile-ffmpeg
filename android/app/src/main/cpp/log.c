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
#include <pthread.h>

/** Log data structure */
struct LogData {
  char *data;
  int level;
  struct LogData *next;
};

/** Log redirection variables */
pthread_mutex_t lockMutex;
pthread_cond_t lockCondition;

pthread_t logThread;
int logRedirectionEnabled;

struct LogData *logDataHead;
struct LogData *logDataTail;

/** Global reference to the virtual machine running */
static JavaVM *globalVm;

/** Global reference of Log class in Java side */
static jclass logClass;

/** Global reference of log redirection method in Java side */
static jmethodID logMethod;

/** Full name of the Java class that owns native functions in this file. */
const char *logClassName = "com/arthenica/mobileffmpeg/Log";

/** Prototypes of native functions defined by this file. */
JNINativeMethod logMethods[] = {
    {"enableNativeRedirection", "()V", (void*) Java_com_arthenica_mobileffmpeg_Log_enableNativeRedirection},
    {"disableNativeRedirection", "()V", (void*) Java_com_arthenica_mobileffmpeg_Log_disableNativeRedirection},
    {"setNativeLevel", "(I)V", (void*) Java_com_arthenica_mobileffmpeg_Log_setNativeLevel},
    {"getNativeLevel", "()I", (void*) Java_com_arthenica_mobileffmpeg_Log_getNativeLevel}
};

void mutexInit() {
    pthread_mutexattr_t attributes;
    pthread_mutexattr_init(&attributes);
    pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE_NP);

    pthread_condattr_t cattributes;
    pthread_condattr_init(&cattributes);
    pthread_condattr_setpshared(&cattributes, PTHREAD_PROCESS_PRIVATE);

    pthread_mutex_init(&lockMutex, &attributes);
    pthread_mutexattr_destroy(&attributes);

    pthread_cond_init(&lockCondition, &cattributes);
    pthread_condattr_destroy(&cattributes);
}

void mutexUnInit() {
    pthread_mutex_destroy(&lockMutex);
    pthread_cond_destroy(&lockCondition);
}

void mutexLock() {
    pthread_mutex_lock(&lockMutex);
}

void mutexUnlock() {
    pthread_mutex_unlock(&lockMutex);
}

void mutexLockWait(int milliSeconds) {
    struct timeval tp;
    struct timespec ts;
    int rc;

    rc = gettimeofday(&tp, NULL);
    if (rc) {
        return;
    }

    ts.tv_sec  = tp.tv_sec;
    ts.tv_nsec = tp.tv_usec * 1000;
    ts.tv_sec += milliSeconds / 1000;
    ts.tv_nsec += (milliSeconds % 1000)*1000000;

    pthread_mutex_lock(&lockMutex);
    pthread_cond_timedwait(&lockCondition, &lockMutex, &ts);
    pthread_mutex_unlock(&lockMutex);
}

void mutexLockNotify() {
    pthread_mutex_lock(&lockMutex);
    pthread_cond_signal(&lockCondition);
    pthread_mutex_unlock(&lockMutex);
}

/**
 * Adds data to the end of log data list.
 */
void logDataAdd(const int level, const char *data) {

    // CREATE DATA STRUCT FIRST
    struct LogData *newData = (struct LogData*)malloc(sizeof(struct LogData));
    newData->level = level;
    newData->data = (char*)malloc(strlen(data));
    strcpy(newData->data, data);
    newData->next = NULL;

    mutexLock();

    // INSERT IT TO THE END OF QUEUE
    if (logDataTail == NULL) {
        logDataTail = newData;

        if (logDataHead != NULL) {
            LOGE("Dangling log data head detected. This can cause memory leak.");
        } else {
            logDataHead = newData;
        }
    } else {
        struct LogData *oldTail = logDataTail;
        oldTail->next = newData;

        logDataTail = newData;
    }

    mutexUnlock();

    mutexLockNotify();
}

/**
 * Removes head of log data list.
 */
struct LogData *logDataRemove() {
    struct LogData *currentData;

    mutexLock();

    if (logDataHead == NULL) {
        currentData = NULL;
    } else {
        currentData = logDataHead;

        struct LogData *nextHead = currentData->next;
        if (nextHead == NULL) {
            if (logDataHead != logDataTail) {
                LOGE("Head and tail log data pointers do not match for single log data element. This can cause memory leak.");
            } else {
                logDataTail = NULL;
            }
            logDataHead = NULL;

        } else {
            logDataHead = nextHead;
        }
    }

    mutexUnlock();

    return currentData;
}

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
        LOGE("OnLoad failed to GetEnv for class %s.\n", logClassName);
        return JNI_FALSE;
    }

    (*env)->GetJavaVM(env, &globalVm);

    jclass clazz = (*env)->FindClass(env, logClassName);
    if (clazz == NULL) {
        LOGE("OnLoad failed to FindClass %s.\n", logClassName);
        return JNI_FALSE;
    }

    logMethod = (*env)->GetStaticMethodID(env, clazz, "log", "(I[B)V");
    if (logMethod == NULL) {
        LOGE("OnLoad thread failed to GetMethodID for %s.\n", "log");
        (*globalVm)->DetachCurrentThread(globalVm);
        return JNI_FALSE;
    }

    if ((*env)->RegisterNatives(env, clazz, logMethods, 4) < 0) {
        LOGE("OnLoad failed to RegisterNatives for class %s.\n", logClassName);
        return JNI_FALSE;
    }

    logClass = (jclass) ((*env)->NewGlobalRef(env, clazz));

    logRedirectionEnabled = 0;

    logDataHead = NULL;
    logDataTail = NULL;

    mutexInit();

    return JNI_VERSION_1_6;
}

/**
 * Callback function for ffmpeg logs.
 *
 * \param pointer to AVClass struct
 * \param level
 * \param format
 * \param arguments
 */
void logCallbackFunction(void *ptr, int level, const char* format, va_list vargs) {
    char line[1024];    // line size is defined as 1024 in libavutil/log.c

    vsnprintf(line, 1024, format, vargs);

    logDataAdd(level, line);
}

/**
 * Forwards log messages to Java classes.
 */
void *logThreadFunction() {
    JNIEnv *env;
    jint getEnvRc = (*globalVm)->GetEnv(globalVm, (void**) &env, JNI_VERSION_1_6);
    if (getEnvRc != JNI_OK) {
        if (getEnvRc != JNI_EDETACHED) {
            LOGE("Log redirect thread failed to GetEnv for class %s with rc %d.\n", logClassName, getEnvRc);
            return NULL;
        }

        if ((*globalVm)->AttachCurrentThread(globalVm, &env, NULL) != 0) {
            LOGE("Log redirect thread failed to AttachCurrentThread for class %s.\n", logClassName);
            return NULL;
        }
    }

    LOGD("Log redirect thread started.\n");

    while(logRedirectionEnabled) {

        struct LogData *logData = logDataRemove();
        if (logData != NULL) {
            size_t size = strlen(logData->data);

            jbyteArray byteArray = (jbyteArray) (*env)->NewByteArray(env, size);
            (*env)->SetByteArrayRegion(env, byteArray, 0, size, (jbyte *)logData->data);
            (*env)->CallStaticVoidMethod(env, logClass, logMethod, logData->level, byteArray);
            (*env)->DeleteLocalRef(env, byteArray);

            // CLEAN LOG DATA STRUCT
            logData->next = NULL;
            logData->level = 0;
            free(logData->data);
            free(logData);
        } else {
            mutexLockWait(100);
        }
    }

    (*globalVm)->DetachCurrentThread(globalVm);

    LOGD("Log redirect thread stopped.\n");

    return NULL;
}

/**
 * Sets log level.
 *
 * \param env pointer to native method interface
 * \param reference to the class on which this method is invoked
 * \param log level
 */
JNIEXPORT void JNICALL Java_com_arthenica_mobileffmpeg_Log_setNativeLevel(JNIEnv *env, jclass object, jint level) {
    av_log_set_level(level);
}

/**
 * Returns current log level.
 *
 * \param env pointer to native method interface
 * \param reference to the class on which this method is invoked
 */
JNIEXPORT jint JNICALL Java_com_arthenica_mobileffmpeg_Log_getNativeLevel(JNIEnv *env, jclass object) {
    return av_log_get_level();
}

/**
 * Enables output redirection.
 *
 * \param env pointer to native method interface
 * \param reference to the class on which this method is invoked
 */
JNIEXPORT void JNICALL Java_com_arthenica_mobileffmpeg_Log_enableNativeRedirection(JNIEnv *env, jclass object) {
    mutexLock();

    if (logRedirectionEnabled != 0) {
        mutexUnlock();
        return;
    }
    logRedirectionEnabled = 1;

    mutexUnlock();

    int rc = pthread_create(&logThread, 0, logThreadFunction, 0);
    if (rc != 0) {
        LOGE("Failed to create log redirect thread (rc=%d).\n", rc);
        return;
    }

    av_log_set_callback(logCallbackFunction);
}

/**
 * Disables output redirection.
 *
 * \param env pointer to native method interface
 * \param reference to the class on which this method is invoked
 */
JNIEXPORT void JNICALL Java_com_arthenica_mobileffmpeg_Log_disableNativeRedirection(JNIEnv *env, jclass object) {

    mutexLock();

    if (logRedirectionEnabled != 1) {
        mutexUnlock();
        return;
    }
    logRedirectionEnabled = 0;

    mutexUnlock();

    av_log_set_callback(av_log_default_callback);

    mutexLockNotify();
}
