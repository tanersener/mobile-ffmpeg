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

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <Foundation/Foundation.h>
#include "LogDelegate.h"
#include "StatisticsDelegate.h"

/** Identifier used for IOS logging. */
extern NSString *const LIB_NAME;

/**
 * Print no output.
 */
#define AV_LOG_QUIET    -8

/**
 * Something went really wrong and we will crash now.
 */
#define AV_LOG_PANIC     0

/**
 * Something went wrong and recovery is not possible.
 * For example, no header was found for a format which depends
 * on headers or an illegal combination of parameters is used.
 */
#define AV_LOG_FATAL     8

/**
 * Something went wrong and cannot losslessly be recovered.
 * However, not all future data is affected.
 */
#define AV_LOG_ERROR    16

/**
 * Something somehow does not look correct. This may or may not
 * lead to problems. An example would be the use of '-vstrict -2'.
 */
#define AV_LOG_WARNING  24

/**
 * Standard information.
 */
#define AV_LOG_INFO     32

/**
 * Detailed information.
 */
#define AV_LOG_VERBOSE  40

/**
 * Stuff which is only useful for libav* developers.
 */
#define AV_LOG_DEBUG    48

/**
 * This class is used to configure MobileFFmpeg library utilities/tools.
 *
 * 1. LogDelegate: By default this class redirects FFmpeg output to NSLog. As another
 * option, it is possible not to print messages to NSLog and pass them to a LogDelegate
 * function. This function can decide whether to print these logs, show them inside another
 * container or ignore them.
 *
 * 2. setLogLevel:/getLogLevel: Use this methods to see/control FFmpeg
 * log severity.
 *
 * 3. StatsDelegate: It is possible to receive statistics about ongoing operation by
 * defining a StatsDelegate or by calling getLastReceivedStats
 * method.
 *
 * 4. Font configuration: It is possible to register custom fonts with
 * setFontconfigConfigurationPath: and
 * setFontDirectory:with: methods.
 *
 * PS: This class is introduced in v2.1 as an enhanced version of older Log class.
 */
@interface MobileFFmpegConfig : NSObject

/**
 * Enables log and statistics redirection.
 * When redirection is not enabled FFmpeg logs are printed to stderr. By enabling redirection, they are routed to
 * NSLog and can be routed further to a log delegate.
 * Statistics redirection behaviour is similar. Statistics are not printed at all if redirection is not enabled.
 * If it is enabled then it is possible to define a statistics delegate but if you don't, they are not printed
 * anywhere and only saved as 'lastReceivedStatistics' data which can be polled with 'getLastReceivedStatistics()'.
 * Note that redirection is enabled by default. If you do not want to use its functionality please use
 * 'disableRedirection()' to disable it.
 */
+ (void)enableRedirection;

/**
 * Disables log and statistics redirection.
 */
+ (void)disableRedirection;

/**
 * Returns log level.
 *
 * @return log level
 */
+ (int)getLogLevel;

/**
 * Sets log level.
 *
 * @param level log level
 */
+ (void)setLogLevel: (int)level;

/**
 * Converts int log level to string.
 *
 * @param level value
 * @return string value
 */
+ (NSString*)logLevelToString: (int)level;

/**
 * Sets a LogDelegate. logCallback method inside LogDelegate is used to redirect logs.
 *
 * @param newLogDelegate log delegate or nil to disable a previously defined delegate
 */
+ (void)setLogDelegate: (id<LogDelegate>)newLogDelegate;

/**
 * Sets a StatisticsDelegate. statisticsCallback method inside StatisticsDelegate is used to redirect statistics.
 *
 * @param newStatisticsDelegate statistics delegate or nil to disable a previously defined delegate
 */
+ (void)setStatisticsDelegate: (id<StatisticsDelegate>)newStatisticsDelegate;

/**
 * Returns the last received statistics data. It is recommended to call it before starting a new execution.
 *
 * @return last received statistics data
 */
+ (Statistics*)getLastReceivedStatistics;

/**
 * Resets last received statistics.
 */
+ (void)resetStatistics;

/**
 * Sets and overrides fontconfig configuration directory.
 *
 * @param path directory which contains fontconfig configuration (fonts.conf)
 */
+ (void)setFontconfigConfigurationPath: (NSString*)path;

/**
 * Registers fonts inside the given path, so they are available to use in FFmpeg filters.
 *
 * Note that you need to build MobileFFmpeg with fontconfig
 * enabled or use a prebuilt package with fontconfig inside to use this feature.
 *
 * @param fontDirectoryPath directory which contains fonts (.ttf and .otf files)
 * @param fontNameMapping custom font name mappings, useful to access your fonts with more friendly names
 */
+ (void)setFontDirectory: (NSString*)fontDirectoryPath with:(NSDictionary*)fontNameMapping;

/**
 * Returns package name.
 *
 * @return guessed package name according to supported external libraries
 */
+ (NSString*)getPackageName;

/**
 * Returns supported external libraries.
 *
 * @return array of supported external libraries
 */
+ (NSArray*)getExternalLibraries;

/**
 * Creates a new named pipe to use in FFmpeg operations.
 *
 * Please note that creator is responsible of closing created pipes.
 *
 * @return the full path of named pipe
 */
+ (NSString*)registerNewFFmpegPipe;

/**
 * Closes a previously created FFmpeg pipe.
 *
 * @param ffmpegPipePath full path of ffmpeg pipe
 */
+ (void)closeFFmpegPipe: (NSString*)ffmpegPipePath;

@end
