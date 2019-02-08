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

/**
 * Stream information class.
 */
@interface StreamInformation : NSObject

- (instancetype)init;

/**
 * Returns stream index.
 *
 * @return stream index, starting from zero
 */
- (NSNumber*)getIndex;

/**
 * Sets stream index.
 *
 * @param index stream index, starting from zero
 */
- (void)setIndex:(NSNumber*) index;

/**
 * Returns stream type.
 *
 * @return stream type; audio or video
 */
- (NSString*)getType;

/**
 * Sets stream type.
 *
 * @param type stream type; audio or video
 */
- (void)setType:(NSString*) type;

/**
 * Returns stream codec.
 *
 * @return stream codec
 */
- (NSString*)getCodec;

/**
 * Sets stream codec.
 *
 * @param codec stream codec
 */
- (void)setCodec:(NSString*) codec;

/**
 * Returns full stream codec.
 *
 * @return stream codec with additional profile and mode information
 */
- (NSString*)getFullCodec;

/**
 * Sets full stream codec.
 *
 * @param fullCodec stream codec with additional profile and mode information
 */
- (void)setFullCodec:(NSString*) fullCodec;

/**
 * Returns stream format.
 *
 * @return stream format
 */
- (NSString*)getFormat;

/**
 * Sets stream format.
 *
 * @param format stream format
 */
- (void)setFormat:(NSString*) format;

/**
 * Returns full stream format.
 *
 * @return stream format with
 */
- (NSString*)getFullFormat;

/**
 * Sets full stream format.
 *
 * @param fullFormat stream format with
 */
- (void)setFullFormat:(NSString*) fullFormat;

/**
 * Returns width.
 *
 * @return width in pixels
 */
- (NSNumber*)getWidth;

/**
 * Sets width.
 *
 * @param width in pixels
 */
- (void)setWidth:(NSNumber*) width;

/**
 * Returns height.
 *
 * @return height in pixels
 */
- (NSNumber*)getHeight;

/**
 * Sets height.
 *
 * @param height in pixels
 */
- (void)setHeight:(NSNumber*) height;

/**
 * Returns bitrate.
 *
 * @return bitrate in kb/s
 */
- (NSNumber*)getBitrate;

/**
 * Sets bitrate.
 *
 * @param bitrate in kb/s
 */
- (void)setBitrate:(NSNumber*) bitrate;

/**
 * Returns sample rate.
 *
 * @return sample rate in hz
 */
- (NSNumber*)getSampleRate;

/**
 * Sets sample rate.
 *
 * @param sampleRate in hz
 */
- (void)setSampleRate:(NSNumber*) sampleRate;

/**
 * Returns sample format.
 *
 * @return sample format
 */
- (NSString*)getSampleFormat;

/**
 * Sets sample format.
 *
 * @param sampleFormat sample format
 */
- (void)setSampleFormat:(NSString*) sampleFormat;

/**
 * Returns channel layout.
 *
 * @return channel layout
 */
- (NSString*)getChannelLayout;

/**
 * Sets channel layout.
 *
 * @param channelLayout mono or stereo
 */
- (void)setChannelLayout:(NSString*) channelLayout;

/**
 * Returns sample aspect ratio.
 *
 * @return sample aspect ratio
 */
- (NSString*)getSampleAspectRatio;

/**
 * Sets sample aspect ratio.
 *
 * @param sampleAspectRatio sample aspect ratio
 */
- (void)setSampleAspectRatio:(NSString*) sampleAspectRatio;

/**
 * Returns display aspect ratio.
 *
 * @return display aspect ratio
 */
- (NSString*)getDisplayAspectRatio;

/**
 * Sets display aspect ratio.
 *
 * @param displayAspectRatio display aspect ratio
 */
- (void)setDisplayAspectRatio:(NSString*) displayAspectRatio;

/**
 * Returns average frame rate.
 *
 * @return average frame rate in fps
 */
- (NSString*)getAverageFrameRate;

/**
 * Sets average frame rate.
 *
 * @param averageFrameRate average frame rate in fps
 */
- (void)setAverageFrameRate:(NSString*) averageFrameRate;

/**
 * Returns real frame rate.
 *
 * @return real frame rate in tbr
 */
- (NSString*)getRealFrameRate;

/**
 * Sets real frame rate.
 *
 * @param realFrameRate real frame rate in tbr
 */
- (void)setRealFrameRate:(NSString*) realFrameRate;

/**
 * Returns time base.
 *
 * @return time base in tbn
 */
- (NSString*)getTimeBase;

/**
 * Sets time base.
 *
 * @param timeBase time base in tbn
 */
- (void)setTimeBase:(NSString*) timeBase;

/**
 * Returns codec time base.
 *
 * @return codec time base in tbc
 */
- (NSString*)getCodecTimeBase;

/**
 * Sets codec time base.
 *
 * @param codecTimeBase codec time base in tbc
 */
- (void)setCodecTimeBase:(NSString*) codecTimeBase;

/**
 * Adds metadata.
 *
 * @param key metadata key
 * @param value metadata value
 */
- (void)addMetadata:(NSString*)key :(NSString*)value;

/**
 * Returns all metadata entries.
 *
 * @return metadata dictionary
 */
- (NSDictionary*)getMetadataEntries;

@end
