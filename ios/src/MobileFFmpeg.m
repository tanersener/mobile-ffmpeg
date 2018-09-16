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

#include "fftools_ffmpeg.h"

#include "MobileFFmpeg.h"
#include "ArchDetect.h"
#include "MobileFFmpegConfig.h"

/** Forward declaration for function defined in fftools_ffmpeg.c */
int execute(int argc, char **argv);

@implementation MobileFFmpeg

/** Global library version */
NSString *const MOBILE_FFMPEG_VERSION = @"2.1.1";

/** Common return code values */
int const RETURN_CODE_SUCCESS = 0;
int const RETURN_CODE_CANCEL = 255;

+ (void)initialize {
    [MobileFFmpegConfig class];
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
 * \param FFmpeg command options/arguments in one string
 * \return zero on successful execution, 255 on user cancel and non-zero on error
 */
+ (int)execute: (NSString*)arguments {

    // SPLITTING ARGUMENTS
    NSArray* argumentArray = [arguments componentsSeparatedByString:@" "];
    return [self executeWithArray:argumentArray];
}

/**
 * Synchronously executes FFmpeg with arguments provided.
 *
 * \param FFmpeg command options/arguments as string array
 * \return zero on successful execution, 255 on user cancel and non-zero on error
 */
+ (int)executeWithArray: (NSArray*)arguments {
    char **commandCharPArray = (char **)malloc(sizeof(char*) * ([arguments count] + 1));

    /* PRESERVING CALLING FORMAT
     *
     * ffmpeg <arguments>
     */
    commandCharPArray[0] = (char *)malloc(sizeof(char) * ([LIB_NAME length] + 1));
    strcpy(commandCharPArray[0], [LIB_NAME UTF8String]);

    for (int i=0; i < [arguments count]; i++) {
        NSString *argument = [arguments objectAtIndex:i];
        commandCharPArray[i + 1] = (char *) [argument UTF8String];
    }

    // RUN
    int retCode = execute(([arguments count] + 1), commandCharPArray);

    // CLEANUP
    free(commandCharPArray[0]);
    free(commandCharPArray);

    return retCode;
}

/**
 * Cancels an ongoing operation.
 *
 * This function does not wait for termination to complete and returns immediately.
 */
+ (void)cancel {
    cancel_operation();
}

@end
