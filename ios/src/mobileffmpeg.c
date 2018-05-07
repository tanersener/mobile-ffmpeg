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

// forward declaration for ffmpeg.c
int execute(int argc, char **argv);

const char *mobileffmpeg_getFFmpegVersion(void) {
    return FFMPEG_VERSION;
}

const char *mobileffmpeg_getVersion(void) {
    return MOBILE_FFMPEG_VERSION;
}

int mobileffmpeg_execute(int argc, char **argv) {
    int argumentCount = 1;
    char **newArgv = NULL;

    if (argv != NULL) {
        argumentCount = argc + 1;
    }

    /* PRESERVING USAGE FORMAT
     *
     * ffmpeg <arguments>
     */
    newArgv = (char **)malloc(sizeof(char*) * (argumentCount));
    newArgv[0] = (char *)malloc(sizeof(char) * (strlen(LIB_NAME) + 1));
    strcpy(newArgv[0], LIB_NAME);

    // PREPARE
    if (argv != NULL) {
        for (int i = 0; i < (argumentCount - 1); i++) {
            newArgv[i + 1] = argv[i];
        }
    }

    // RUN
    int retCode = execute(argumentCount, newArgv);

    // CLEANUP
    free(newArgv[0]);
    free(newArgv);

    return retCode;
}
