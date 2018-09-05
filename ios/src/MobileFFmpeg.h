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
#include "libavutil/ffversion.h"

/** Global library version */
extern NSString *const MOBILE_FFMPEG_VERSION;

/** Common return code values */
extern int const RETURN_CODE_SUCCESS;
extern int const RETURN_CODE_CANCEL;

/**
 * Main class for FFmpeg operations.
 */
@interface MobileFFmpeg : NSObject

/**
 * Returns FFmpeg version bundled within the library.
 *
 * \return FFmpeg version
 */
+ (NSString*)getFFmpegVersion;

/**
 * Returns MobileFFmpeg library version.
 *
 * \return MobileFFmpeg version
 */
+ (NSString*)getVersion;

/**
 * Synchronously executes FFmpeg with arguments provided.
 *
 * \param FFmpeg command options/arguments in one string
 * \return zero on successful execution, 255 on user cancel and non-zero on error
 */
+ (int)execute: (NSString*)arguments;

/**
 * Synchronously executes FFmpeg with arguments provided.
 *
 * \param FFmpeg command options/arguments as string array
 * \return zero on successful execution, 255 on user cancel and non-zero on error
 */
+ (int)executeWithArray: (NSArray*)arguments;

/**
 * Cancels an ongoing operation.
 *
 * This function does not wait for termination to complete and returns immediately.
 */
+ (void)cancel;

@end