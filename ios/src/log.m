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

#include "log.h"
#import <Foundation/Foundation.h>

/**
 * Verbose logging function.
 *
 * \param message log message
 */
void logv(const char *message,...) {
    va_list args;
    va_start(args, message);
    NSLog(@"%@", [[NSString alloc] initWithFormat:[NSString stringWithUTF8String:message] arguments:args]);
    va_end(args);
}

/**
 * Debug logging function.
 *
 * \param message log message
 */
void logd(const char *message,...) {
    va_list args;
    va_start(args, message);
    NSLog(@"%@", [[NSString alloc] initWithFormat:[NSString stringWithUTF8String:message] arguments:args]);
    va_end(args);
}

/**
 * Info logging function.
 *
 * \param message log message
 */
void logi(const char *message,...) {
    va_list args;
    va_start(args, message);
    NSLog(@"%@", [[NSString alloc] initWithFormat:[NSString stringWithUTF8String:message] arguments:args]);
    va_end(args);
}

/**
 * Warn logging function.
 *
 * \param message log message
 */
void logw(const char *message,...) {
    va_list args;
    va_start(args, message);
    NSLog(@"%@", [[NSString alloc] initWithFormat:[NSString stringWithUTF8String:message] arguments:args]);
    va_end(args);
}

/**
 * Error logging function.
 *
 * \param message log message
 */
void loge(const char *message,...) {
    va_list args;
    va_start(args, message);
    NSLog(@"%@", [[NSString alloc] initWithFormat:[NSString stringWithUTF8String:message] arguments:args]);
    va_end(args);
}
