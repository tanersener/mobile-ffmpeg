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

#include "MediaInformation.h"

@implementation MediaInformation {

    /**
     * Format
     */
    NSString *format;
    
    /**
     * Path
     */
    NSString *path;
    
    /**
     * Duration, in milliseconds
     */
    NSNumber *duration;
    
    /**
     * Start time, in milliseconds
     */
    NSNumber *startTime;
    
    /**
     * Bitrate, kb/s
     */
    NSNumber *bitrate;
    
    /**
     * Metadata map
     */
    NSMutableDictionary *metadata;
    
    /**
     * List of streams
     */
    NSMutableArray *streams;
    
    /**
     * Raw unparsed media information
     */
    NSString *rawInformation;

}

- (instancetype)init {
    self = [super init];
    if (self) {
        format = nil;
        path = nil;
        duration = nil;
        startTime = nil;
        bitrate = nil;
        metadata = [[NSMutableDictionary alloc] init];
        streams = [[NSMutableArray alloc] init];
        rawInformation = nil;
    }
    
    return self;
}

- (NSString*)getFormat {
    return format;
}

- (void)setFormat:(NSString*)newFormat {
    format = newFormat;
}

- (NSString*)getPath {
    return path;
}

- (void)setPath:(NSString*)newPath {
    path = newPath;
}

- (NSNumber*)getDuration {
    return duration;
}

- (void)setDuration:(NSNumber*)newDuration {
    duration = newDuration;
}

- (NSNumber*)getStartTime {
    return startTime;
}

- (void)setStartTime:(NSNumber*)newStartTime {
    startTime = newStartTime;
}

- (NSNumber*)getBitrate {
    return bitrate;
}

- (void)setBitrate:(NSNumber*)newBitrate {
    bitrate = newBitrate;
}

- (NSString*)getRawInformation {
    return rawInformation;
}

- (void)setRawInformation:(NSString*)newRawInformation {
    rawInformation = newRawInformation;
}

- (void)addMetadata:(NSString*)key :(NSString*)value {
    metadata[key] = value;
}

- (NSDictionary*)getMetadataEntries {
    return metadata;
}

- (void)addStream:(StreamInformation*)stream {
    [streams addObject:stream];
}

- (NSArray*)getStreams {
    return streams;
}

@end
