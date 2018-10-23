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
#include "MediaInformation.h"

@interface MediaInformationParser : NSObject

/**
 * Extracts MediaInformation from given command output.
 */
+ (MediaInformation*)from: (NSString*)rawCommandOutput;

/**
 * Extracts StreamInformation from given input.
 */
+ (StreamInformation*)parseStreamBlock:(NSString*)input;

+ (void (^)(NSString *__autoreleasing*, NSString *__autoreleasing*))parseInputBlock:(NSString*)input;
+ (void (^)(NSNumber *__autoreleasing*, NSNumber *__autoreleasing*, NSNumber *__autoreleasing*))parseDurationBlock:(NSString*)input;
+ (void (^)(NSString *__autoreleasing*, NSString *__autoreleasing*))parseMetadataBlock:(NSString*)input;
+ (void (^)(NSNumber *__autoreleasing*, NSNumber *__autoreleasing*))parseVideoDimensions: (NSString*)input;

+ (NSString*)parseVideoStreamSampleAspectRatio: (NSString*)input;
+ (NSString*)parseVideoStreamDisplayAspectRatio: (NSString*)input;
+ (NSNumber*)parseAudioStreamSampleRate: (NSString*)input;
+ (NSString*)parseStreamType: (NSString*)input;
+ (NSString*)parseStreamCodec: (NSString*)input;
+ (NSString*)parseStreamFullCodec: (NSString*)input;
+ (NSNumber*)parseStreamIndex: (NSString*)input;
+ (NSNumber*)parseDuration: (NSString*)input;
+ (NSNumber*)parseStartTime: (NSString*)input;
+ (NSString*)substring:(NSString*)string from:(NSString*)start to:(NSString*)end ignoring:(NSArray*)ignoredTokens;
+ (NSString*)substring:(NSString*)string from:(NSString*)start ignoring:(NSArray*)ignoredTokens;
+ (NSString*)substring:(NSString*)string to:(NSString*)start ignoring:(NSArray*)ignoredTokens;
+ (int)index:(NSString*)string of:(NSString*)substring from:(int)startIndex times:(int)n;
+ (int)count:(NSString*)string of:(NSString*)substring;
+ (NSNumber*)toInteger: (NSString*)input;
+ (NSNumber*)toIntegerObject: (NSString*)input;
+ (NSString*)safeGet:(NSArray*)array from:(int)index;

@end
