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

#include "Log.h"
#include "ArchDetect.h"
#include "MobileFFmpeg.h"

/** Holds delegate defined to redirect logs */
static id<LogDelegate> logDelegate = nil;

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

    int activeLogLevel = [Log getLevel];

    if (activeLogLevel == AV_LOG_QUIET || level > activeLogLevel) {
        // LOG NEITHER PRINTED NOR FORWARDED
        return;
    }

    vsnprintf(line, 1024, format, vargs);

    NSString* logMessage = [NSString stringWithCString:line encoding:NSUTF8StringEncoding];

    if (logDelegate != nil) {

        // FORWARD LOG TO LOG DELEGATE
        [logDelegate logCallback:level:logMessage];
    } else {

        // WRITE TO NSLOG
        NSLog(@"%@: %@", [Log levelToString:level], logMessage);
    }
}

@implementation Log

NSString *const LOG_LIB_NAME = @"mobile-ffmpeg";

+ (void)initialize {
    [ArchDetect class];
    [MobileFFmpeg class];

    [Log enableRedirection];
}

/**
 * Enables log redirection.
 */
+ (void)enableRedirection {
    av_log_set_callback(logCallbackFunction);
}

/**
 * Disables log redirection.
 */
+ (void)disableRedirection {
    av_log_set_callback(av_log_default_callback);
}

/**
 * Returns log level.
 *
 * \return log level
 */
+ (int)getLevel {
    return av_log_get_level();
}

/**
 * Sets log level.
 *
 * \param log level
 */
+ (void)setLevel: (int)level {
    av_log_set_level(level);
}

/**
 * Convert int log level to string.
 *
 * \param level value
 * \return string value
 */
+ (NSString*)levelToString: (int)level {
    switch (level) {
        case AV_LOG_TRACE: return @"TRACE";
        case AV_LOG_DEBUG: return @"DEBUG";
        case AV_LOG_VERBOSE: return @"VERBOSE";
        case AV_LOG_INFO: return @"INFO";
        case AV_LOG_WARNING: return @"WARNING";
        case AV_LOG_ERROR: return @"ERROR";
        case AV_LOG_FATAL: return @"FATAL";
        case AV_LOG_PANIC: return @"PANIC";
        case AV_LOG_QUIET:
        default: return @"";
    }
}

/**
 * Sets a LogDelegate. logCallback method inside LogDelegate is used to redirect logs.
 *
 * \param new log delegate
 */
+ (void)setLogDelegate: (id<LogDelegate>)newLogDelegate {
    logDelegate = newLogDelegate;
}

@end
