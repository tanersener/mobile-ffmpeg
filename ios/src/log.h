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

#ifndef MOBILEFFMPEG_LOG_H
#define MOBILEFFMPEG_LOG_H

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

/** Identifier used for IOS logging. */
#define LIB_NAME "mobile-ffmpeg"

/**
 * Verbose logging function.
 *
 * \param message log message
 */
void logv(const char *message, ...) __attribute__((format(printf, 1, 2)));

/**
 * Debug logging function.
 *
 * \param message log message
 */
void logd(const char *message, ...) __attribute__((format(printf, 1, 2)));

/**
 * Info logging function.
 *
 * \param message log message
 */
void logi(const char *message, ...) __attribute__((format(printf, 1, 2)));

/**
 * Warn logging function.
 *
 * \param message log message
 */
void logw(const char *message, ...) __attribute__((format(printf, 1, 2)));

/**
 * Error logging function.
 *
 * \param message log message
 */
void loge(const char *message, ...) __attribute__((format(printf, 1, 2)));

#endif /* MOBILEFFMPEG_LOG_H */
