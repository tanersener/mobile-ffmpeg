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
#include "libavutil/log.h"

/** Identifier used for IOS logging. */
extern NSString *const LOG_LIB_NAME;

/**
 * This class is used to process FFmpeg logs.
 */
@interface Log : NSObject

/**
 * Enables log redirection.
 */
+ (void)enableRedirection;

/**
 * Disables log redirection.
 */
+ (void)disableRedirection;

/**
 * Returns log level.
 *
 * \return log level
 */
+ (int)getLevel;

/**
 * Sets log level.
 *
 * \param log level
 */
+ (void)setLevel: (int)level;

/**
 * Convert int log level to string.
 *
 * \param level value
 * \return string value
 */
+ (NSString*)levelToString: (int)level;

/**
 * Sets a LogDelegate. logCallback method inside LogDelegate is used to redirect logs.
 *
 * \param log delegate
 */
+ (void)setLogDelegate: (id<LogDelegate>)newLogDelegate;

@end
