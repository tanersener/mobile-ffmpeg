/*
 * Copyright (c) 2020 Taner Sener
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

#include "MediaInformationParser.h"

/**
 * Main class for FFprobe operations.
 */
@interface MobileFFprobe : NSObject

/**
 * Synchronously executes FFprobe with arguments provided.
 *
 * @param arguments FFprobe command options/arguments as string array
 * @return zero on successful execution, 255 on user cancel and non-zero on error
 */
+ (int)executeWithArguments: (NSArray*)arguments;

/**
 * Synchronously executes FFprobe command provided. Space character is used to split command
 * into arguments.
 *
 * @param command FFprobe command
 * @return zero on successful execution, 255 on user cancel and non-zero on error
 */
+ (int)execute: (NSString*)command;

/**
 * Returns media information for given file.
 *
 * @param path file path or uri of media file
 * @return media information
 */
+ (MediaInformation*)getMediaInformation: (NSString*)path;

/**
 * Returns media information for given file.
 *
 * @param path path or uri of media file
 * @param timeout complete timeout
 * @return media information
 */
+ (MediaInformation*)getMediaInformation: (NSString*)path timeout:(long)timeout;

@end