/*
 * Copyright (c) 2018, 2020 Taner Sener
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

/**
 * Media information class.
 */
@interface MediaInformation : NSObject

- (instancetype)init:(NSDictionary*)mediaDictionary withStreams:(NSArray*)streams;

/**
 * Returns file name.
 *
 * @return media file name
 */
- (NSString*)getFilename;

/**
 * Returns format.
 *
 * @return media format
 */
- (NSString*)getFormat;

/**
 * Returns long format.
 *
 * @return media long format
 */
- (NSString*)getLongFormat;

/**
 * Returns duration.
 *
 * @return media duration in milliseconds
 */
- (NSString*)getDuration;

/**
 * Returns start time.
 *
 * @return media start time in milliseconds
 */
- (NSString*)getStartTime;

/**
 * Returns size.
 *
 * @return media size in bytes
 */
- (NSString*)getSize;

/**
 * Returns bitrate.
 *
 * @return media bitrate in kb/s
 */
- (NSString*)getBitrate;

/**
 * Returns all tags.
 *
 * @return tags dictionary
 */
- (NSDictionary*)getTags;

/**
 * Returns all streams.
 *
 * @return streams array
 */
- (NSArray*)getStreams;

/**
 * Returns the media property associated with the key.
 *
 * @return media property as string or nil if the key is not found
 */
- (NSString*)getStringProperty:(NSString*)key;

/**
 * Returns the media property associated with the key.
 *
 * @return media property as number or nil if the key is not found
 */
- (NSNumber*)getNumberProperty:(NSString*)key;

/**
 * Returns the media properties associated with the key.
 *
 * @return media properties in a dictionary or nil if the key is not found
*/
- (NSDictionary*)getProperties:(NSString*)key;

/**
 * Returns all media properties.
 *
 * @return all media properties in a dictionary or nil if no media properties are defined
*/
- (NSDictionary*)getMediaProperties;

/**
 * Returns all properties defined.
 *
 * @return all properties in a dictionary or nil if no properties are defined
*/
- (NSDictionary*)getAllProperties;

@end
