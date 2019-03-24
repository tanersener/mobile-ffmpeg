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
NSString *const MOBILE_FFMPEG_VERSION = @"4.2";

/** Common return code values */
int const RETURN_CODE_SUCCESS = 0;
int const RETURN_CODE_CANCEL = 255;

int lastReturnCode;
NSMutableString *lastCommandOutput;

extern NSMutableString *systemCommandOutput;
extern int mobileffmpeg_system_execute(NSArray *arguments, NSArray *commandOutputEndPatternList, long timeout);

+ (void)initialize {
    [MobileFFmpegConfig class];

    lastReturnCode = 0;
    lastCommandOutput = [[NSMutableString alloc] init];

    NSLog(@"Loaded mobile-ffmpeg-%@-%@-%@-%@\n", [MobileFFmpegConfig getPackageName], [ArchDetect getArch], [MobileFFmpeg getVersion], [MobileFFmpeg getBuildDate]);
}

/**
 * Returns FFmpeg version bundled within the library.
 *
 * @return FFmpeg version string
 */
+ (NSString*)getFFmpegVersion {
    return [NSString stringWithUTF8String:FFMPEG_VERSION];
}

/**
 * Returns MobileFFmpeg library version.
 *
 * @return MobileFFmpeg version string
 */
+ (NSString*)getVersion {
    if ([ArchDetect isLTSBuild] == 1) {
        return [NSString stringWithFormat:@"%@-lts", MOBILE_FFMPEG_VERSION];
    } else {
        return MOBILE_FFMPEG_VERSION;
    }
}

/**
 * Synchronously executes FFmpeg with arguments provided.
 *
 * @param arguments FFmpeg command options/arguments as string array
 * @return zero on successful execution, 255 on user cancel and non-zero on error
 */
+ (int)executeWithArguments: (NSArray*)arguments {
    lastCommandOutput = [[NSMutableString alloc] init];

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
    lastReturnCode = execute(([arguments count] + 1), commandCharPArray);

    // CLEANUP
    free(commandCharPArray[0]);
    free(commandCharPArray);

    return lastReturnCode;
}

/**
 * Synchronously executes FFmpeg command provided. Space character is used to split command
 * into arguments.
 *
 * @param command FFmpeg command
 * @return zero on successful execution, 255 on user cancel and non-zero on error
 */
+ (int)execute: (NSString*)command {
    return [self execute:command delimiter:@" "];
}

/**
 * Synchronously executes FFmpeg command provided. Delimiter parameter is used to split
 * command into arguments.
 *
 * @param command FFmpeg command
 * @param delimiter arguments delimiter
 * @return zero on successful execution, 255 on user cancel and non-zero on error
 */
+ (int)execute: (NSString*)command delimiter:(NSString*)delimiter {

    // SPLITTING ARGUMENTS
    NSArray* argumentArray = [command componentsSeparatedByString:(delimiter == nil ? @" ": delimiter)];
    return [self executeWithArguments:argumentArray];
}

/**
 * Cancels an ongoing operation.
 *
 * This function does not wait for termination to complete and returns immediately.
 */
+ (void)cancel {
    cancel_operation();
}

/**
 * Returns return code of last executed command.
 *
 * @return return code of last executed command
 */
+ (int)getLastReturnCode {
    return lastReturnCode;
}

/**
 * Returns log output of last executed command. Please note that disabling redirection using
 * MobileFFmpegConfig.disableRedirection() method also disables this functionality.
 *
 * @return output of last executed command
 */
+ (NSString*)getLastCommandOutput {
    return lastCommandOutput;
}

/**
 * Returns media information for given file.
 *
 * @param path or uri of media file
 * @return media information
 */
+ (MediaInformation*)getMediaInformation: (NSString*)path {
    return [MobileFFmpeg getMediaInformation:path timeout:10000];
}

/**
 * Returns media information for given file.
 *
 * @param path    path or uri of media file
 * @param timeout complete timeout
 * @return media information
 */
 + (MediaInformation*)getMediaInformation: (NSString*)path timeout:(long)timeout {
    int rc = mobileffmpeg_system_execute([[NSArray alloc] initWithObjects:@"-v", @"info", @"-hide_banner", @"-i", path, @"-f", @"null", @"-", nil], [[NSArray alloc] initWithObjects:@"Press [q] to stop, [?] for help", @"No such file or directory", @"Input/output error", @"Conversion failed", nil], timeout);

    if (rc == 0) {
        return [MediaInformationParser from:systemCommandOutput];
    } else {
        int activeLogLevel = av_log_get_level();
        if ((activeLogLevel != AV_LOG_QUIET) && (AV_LOG_WARNING <= activeLogLevel)) {
            NSLog(@"%@", systemCommandOutput);
        }

        return nil;
    }
}

/**
 * Returns MobileFFmpeg library build date.
 *
 * @return MobileFFmpeg library build date
 */
+ (NSString*)getBuildDate {
    char buildDate[10];
    sprintf(buildDate, "%d", MOBILE_FFMPEG_BUILD_DATE);
    return [NSString stringWithUTF8String:buildDate];
}

@end
