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

static int pipeFd[2];
static pthread_t logThread;
static int logThreadEnabled = 1;

static void (*log_callback)(const char *) = NULL;

void set_log_callback(void (*cb)(const char *)) {
    log_callback = cb;
}

static void *logThreadFunction() {
    int readSize;
    char buffer[512];

    LOGI("Native log thread started.");

    while(logThreadEnabled && ((readSize = (int)read(pipeFd[0], buffer, sizeof(buffer) - 1)) > 0)) {
        if (readSize > 0) {
            if (buffer[readSize - 1] == '\n') {
                readSize--;
            }
            buffer[readSize] = 0;  /* add null-terminator */

            if (log_callback == NULL) {
                LOGD("%s", buffer);
            } else {
                log_callback(buffer);
            }
        }
    }

    LOGI("Native log thread stopped.");

    return 0;
}

int startNativeCollector() {

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

int stopNativeCollector() {
    logThreadEnabled = 0;

    LOGI("Stopping native log thread\n");

    return 0;
}
