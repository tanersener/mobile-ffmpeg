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

#include <Foundation/Foundation.h>
#include "StreamInformation.h"

@interface MediaInformation : NSObject

- (instancetype)init;

/**
 * Returns format.
 *
 * \return media format
 */
- (NSString*)getFormat;

/**
 * Sets media format.
 *
 * \param media format
 */
- (void)setFormat:(NSString*)format;

/**
 * Returns path.
 *
 * \return media path
 */
- (NSString*)getPath;

/**
 * Sets media path.
 *
 * \param media path
 */
- (void)setPath:(NSString*)path;

/**
 * Returns duration.
 *
 * \return media duration in milliseconds
 */
- (NSNumber*)getDuration;

/**
 * Sets media duration.
 *
 * \param media duration in milliseconds
 */
- (void)setDuration:(NSNumber*) duration;

/**
 * Returns start time.
 *
 * \return media start time in milliseconds
 */
- (NSNumber*)getStartTime;

/**
 * Sets media start time.
 *
 * \param media start time in milliseconds
 */
- (void)setStartTime:(NSNumber*)startTime;

/**
 * Returns bitrate.
 *
 * \return media bitrate in kb/s
 */
- (NSNumber*)getBitrate;

/**
 * Sets bitrate.
 *
 * \param media bitrate in kb/s
 */
- (void)setBitrate:(NSNumber*) bitrate;

/**
 * Returns unparsed media information.
 *
 * \return unparsed media information data
 */
- (NSString*)getRawInformation;

/**
 * Sets unparsed media information.
 *
 * \param unparsed media information data
 */
- (void)setRawInformation:(NSString*)rawInformation;

/**
 * Adds metadata.
 *
 * \param metadata key and value
 */
- (void)addMetadata:(NSString*)key :(NSString*)value;

/**
 * Returns all metadata entries.
 *
 * \return metadata dictionary
 */
- (NSDictionary*)getMetadataEntries;

/**
 * Adds new stream.
 *
 * \param new stream information
 */
- (void)addStream:(StreamInformation*) stream;

/**
 * Returns all streams
 *
 * \return streams array
 */
- (NSArray*)getStreams;

@end
