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

#include <string.h>
#include <stdlib.h>
#include <Foundation/Foundation.h>

/** Global library version */
extern NSString *const MOBILE_FFMPEG_VERSION;

/**
 * Main class for FFmpeg operations.
 */
@interface MobileFFmpeg : NSObject

/**
 * Synchronously executes FFmpeg with arguments provided.
 *
 * @param arguments FFmpeg command options/arguments as string array
 * @return zero on successful execution, 255 on user cancel and non-zero on error
 */
+ (int)executeWithArguments: (NSArray*)arguments;

/**
 * Synchronously executes FFmpeg command provided. Space character is used to split command
 * into arguments.
 *
 * @param command FFmpeg command
 * @return zero on successful execution, 255 on user cancel and non-zero on error
 */
+ (int)execute: (NSString*)command;

/**
 * Synchronously executes FFmpeg command provided. Delimiter parameter is used to split
 * command into arguments.
 *
 * @param command FFmpeg command
 * @param delimiter arguments delimiter
 * @deprecated argument splitting mechanism used in this method is pretty simple and prone to errors. Consider
 * using a more advanced method like execute or executeWithArguments
 * @return zero on successful execution, 255 on user cancel and non-zero on error
 */
+ (int)execute: (NSString*)command delimiter:(NSString*)delimiter;

/**
 * Cancels an ongoing operation.
 *
 * This function does not wait for termination to complete and returns immediately.
 */
+ (void)cancel;

/**
 * Parses the given command into arguments.
 *
 * @param command string command
 * @return array of arguments
 */
+ (NSArray*)parseArguments: (NSString*)command;

@end