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

#include "StreamInformation.h"

@implementation StreamInformation {

    /**
     * Stream index
     */
    NSNumber *index;
    
    NSString *type;
    NSString *codec;
    NSString *fullCodec;
    NSString *format;
    NSString *fullFormat;
    
    NSNumber *width;
    NSNumber *height;
    
    NSNumber *bitrate;
    NSNumber *sampleRate;
    NSString *sampleFormat;
    NSString *channelLayout;
    
    /**
     * SAR
     */
    NSString *sampleAspectRatio;
    
    /**
     * DAR
     */
    NSString *displayAspectRatio;
    
    /**
     * fps
     */
    NSString *averageFrameRate;
    
    /**
     * tbr
     */
    NSString *realFrameRate;
    
    /**
     * tbn
     */
    NSString *timeBase;
    
    /**
     * tbc
     */
    NSString *codecTimeBase;
    
    /**
     * Metadata map
     */
    NSMutableDictionary *metadata;

}

- (instancetype)init {
    self = [super init];
    if (self) {
        index = nil;
        type = nil;
        codec = nil;
        fullCodec = nil;
        format = nil;
        fullFormat = nil;
        width = nil;
        height = nil;
        bitrate = nil;
        sampleRate = nil;
        sampleFormat = nil;
        channelLayout = nil;
        sampleAspectRatio = nil;
        displayAspectRatio = nil;
        averageFrameRate = nil;
        realFrameRate = nil;
        timeBase = nil;
        codecTimeBase = nil;
        metadata = nil;
    }
    
    return self;
}

- (NSNumber*)getIndex {
    return index;
}

- (void)setIndex:(NSNumber*) newIndex {
    index = newIndex;
}

- (NSString*)getType {
    return type;
}

- (void)setType:(NSString*) newType {
    type = newType;
}

- (NSString*)getCodec {
    return codec;
}

- (void)setCodec:(NSString*) newCodec {
    codec = newCodec;
}

- (NSString*)getFullCodec {
    return fullCodec;
}

- (void)setFullCodec:(NSString*) newFullCodec {
    fullCodec = newFullCodec;
}

- (NSString*)getFormat {
    return format;
}

- (void)setFormat:(NSString*) newFormat {
    format = newFormat;
}

- (NSString*)getFullFormat {
    return fullFormat;
}

- (void)setFullFormat:(NSString*) newFullFormat {
    fullFormat = newFullFormat;
}

- (NSNumber*)getWidth {
    return width;
}

- (void)setWidth:(NSNumber*) newWidth {
    width = newWidth;
}

- (NSNumber*)getHeight {
    return height;
}

- (void)setHeight:(NSNumber*) newHeight {
    height = newHeight;
}

- (NSNumber*)getBitrate {
    return bitrate;
}

- (void)setBitrate:(NSNumber*) newBitrate {
    bitrate = newBitrate;
}

- (NSNumber*)getSampleRate {
    return sampleRate;
}

- (void)setSampleRate:(NSNumber*) newSampleRate {
    sampleRate = newSampleRate;
}

- (NSString*)getSampleFormat {
    return sampleFormat;
}

- (void)setSampleFormat:(NSString*) newSampleFormat {
    sampleFormat = newSampleFormat;
}

- (NSString*)getChannelLayout {
    return channelLayout;
}

- (void)setChannelLayout:(NSString*) newChannelLayout {
    channelLayout = newChannelLayout;
}

- (NSString*)getSampleAspectRatio {
    return sampleAspectRatio;
}

- (void)setSampleAspectRatio:(NSString*) newSampleAspectRatio {
    sampleAspectRatio = newSampleAspectRatio;
}

- (NSString*)getDisplayAspectRatio {
    return displayAspectRatio;
}

- (void)setDisplayAspectRatio:(NSString*) newDisplayAspectRatio {
    displayAspectRatio = newDisplayAspectRatio;
}

- (NSString*)getAverageFrameRate {
    return averageFrameRate;
}

- (void)setAverageFrameRate:(NSString*) newAverageFrameRate {
    averageFrameRate = newAverageFrameRate;
}

- (NSString*)getRealFrameRate {
    return realFrameRate;
}

- (void)setRealFrameRate:(NSString*) newRealFrameRate {
    realFrameRate = newRealFrameRate;
}

- (NSString*)getTimeBase {
    return timeBase;
}

- (void)setTimeBase:(NSString*) newTimeBase {
    timeBase = newTimeBase;
}

- (NSString*)getCodecTimeBase {
    return codecTimeBase;
}

- (void)setCodecTimeBase:(NSString*) newCodecTimeBase {
    codecTimeBase = newCodecTimeBase;
}

- (void)addMetadata:(NSString*)key :(NSString*)value {
    metadata[key] = value;
}

- (NSDictionary*)getMetadataEntries {
    return metadata;
}

@end
