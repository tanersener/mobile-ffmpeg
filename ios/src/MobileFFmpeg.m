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

#include "MobileFFmpeg.h"
#include "ArchDetect.h"
#include "Log.h"

/** Forward declaration for function defined in ffmpeg.c */
int execute(int argc, char **argv);

@implementation MobileFFmpeg

/** Library version string */
NSString *const MOBILE_FFMPEG_VERSION = @"2.1";

+ (void)initialize {
    [Log class];
    NSLog(@"Loaded mobile-ffmpeg-%@-%@\n", [ArchDetect getArch], [MobileFFmpeg getVersion]);
}

/**
 * Returns FFmpeg version bundled within the library.
 *
 * \return FFmpeg version string
 */
+ (NSString*)getFFmpegVersion {
    return [NSString stringWithUTF8String:FFMPEG_VERSION];
}

/**
 * Returns MobileFFmpeg library version.
 *
 * \return MobileFFmpeg version string
 */
+ (NSString*)getVersion {
    return MOBILE_FFMPEG_VERSION;
}

/**
 * Synchronously executes FFmpeg command with arguments provided.
 *
 * \param argc argument count
 * \param argv arguments pointer
 * \return zero on successful execution, non-zero on error
 */
+ (int)execute: (NSString*)arguments {

    // SPLITTING ARGUMENTS
    NSArray* commandArray = [arguments componentsSeparatedByString:@" "];
    char **commandCharPArray = (char **)malloc(sizeof(char*) * ([commandArray count] + 1));

    /* PRESERVING USAGE FORMAT
     *
     * ffmpeg <arguments>
     */
    commandCharPArray[0] = (char *)malloc(sizeof(char) * ([LOG_LIB_NAME length] + 1));
    strcpy(commandCharPArray[0], [LOG_LIB_NAME UTF8String]);

    for (int i=0; i < [commandArray count]; i++) {
        NSString *argument = [commandArray objectAtIndex:i];
        commandCharPArray[i + 1] = (char *) [argument UTF8String];
    }

    // RUN
    int retCode = execute(([commandArray count] + 1), commandCharPArray);

    // CLEANUP
    free(commandCharPArray[0]);
    free(commandCharPArray);

    return retCode;
}

@end
