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

#include "MediaInformationParser.h"

static NSDateFormatter* durationFormat;
static NSDate* referenceDuration;
static NSRegularExpression *parenthesesRegex;
static NSRegularExpression *bracketsRegex;

@implementation MediaInformationParser

+ (void)initialize {
    NSError *localError = nil;
    durationFormat = [[NSDateFormatter alloc] init];
    [durationFormat setDateFormat:@"HH:mm:ss"];
    
    referenceDuration = [durationFormat dateFromString: @"00:00:00"];
    parenthesesRegex = [NSRegularExpression regularExpressionWithPattern:@"\\(.*\\)" options:NSRegularExpressionCaseInsensitive error:&localError];
    bracketsRegex = [NSRegularExpression regularExpressionWithPattern:@"\\[.*\\]" options:NSRegularExpressionCaseInsensitive error:&localError];
}

+ (MediaInformation*)from: (NSString*)rawCommandOutput {
    MediaInformation* mediaInformation = [[MediaInformation alloc] init];
    
    if (rawCommandOutput != nil) {
        NSArray* split = [rawCommandOutput componentsSeparatedByString:@"\n"];
        Boolean metadata = false;
        Boolean sidedata = false;
        StreamInformation *lastCreatedStream = nil;
        NSMutableString *rawInformation = [[NSMutableString alloc] init];

        for (int i=0; i < [split count]; i++) {
            NSString *outputLine = [split objectAtIndex:i];

            if ([outputLine hasPrefix:@"["]) {
                metadata = false;
                sidedata = false;
                continue;
            }

            NSString *trimmedLine = [outputLine stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
            
            if ([trimmedLine hasPrefix:@"Input"]) {
                metadata = false;
                sidedata = false;
                lastCreatedStream = nil;

                void(^blockPair)(NSString **, NSString **) = [MediaInformationParser parseInputBlock:trimmedLine];

                NSString *format;
                NSString *path;
                blockPair(&format, &path);

                [mediaInformation setFormat:format];
                [mediaInformation setPath:path];
            } else if ([trimmedLine hasPrefix:@"Duration"]) {
                metadata = false;
                sidedata = false;
                lastCreatedStream = nil;

                void(^blockTrio)(NSNumber **, NSNumber **, NSNumber **) = [MediaInformationParser parseDurationBlock:trimmedLine];

                NSNumber *duration;
                NSNumber *startTime;
                NSNumber *bitrate;
                
                blockTrio(&duration, &startTime, &bitrate);
                
                [mediaInformation setDuration:duration];
                [mediaInformation setStartTime:startTime];
                [mediaInformation setBitrate:bitrate];
            } else if ([trimmedLine hasPrefix:@"Metadata"]) {
                sidedata = false;
                metadata = true;
            } else if ([trimmedLine hasPrefix:@"Side data"]) {
                metadata = false;
                sidedata = true;
            } else if ([trimmedLine hasPrefix:@"Stream mapping"] || [trimmedLine hasPrefix:@"Press [q] to stop"] || [trimmedLine hasPrefix:@"Output"]) {
                break;
            } else if ([trimmedLine hasPrefix:@"Stream"]) {
                metadata = false;
                sidedata = false;
                lastCreatedStream = [MediaInformationParser parseStreamBlock:trimmedLine];
                [mediaInformation addStream:lastCreatedStream];
            } else if (metadata) {
                void(^blockPair)(NSString **, NSString **) = [MediaInformationParser parseMetadataBlock:trimmedLine];
                
                NSString *key;
                NSString *value;
                blockPair(&key, &value);

                if (key != nil && value != nil) {
                    if (lastCreatedStream != nil) {
                        [lastCreatedStream addMetadata:key:value];
                    } else {
                        [mediaInformation addMetadata:key:value];
                    }
                }
            } else if (sidedata) {
                void(^blockPair)(NSString **, NSString **) = [MediaInformationParser parseMetadataBlock:trimmedLine];

                NSString *key;
                NSString *value;
                blockPair(&key, &value);

                if (key != nil && value != nil) {
                    if (lastCreatedStream != nil) {
                        [lastCreatedStream addSidedata:key:value];
                    }
                }
            }

            [rawInformation appendString:outputLine];
            [rawInformation appendString:@"\n"];
        }
        
        [mediaInformation setRawInformation:rawInformation];
    }
    
    return mediaInformation;
}

+ (void (^)(NSString *__autoreleasing*, NSString *__autoreleasing*))parseInputBlock:(NSString*)input {
    NSString *format = [MediaInformationParser substring:input from:@"," to:@", from" ignoring:nil];
    NSString *path = [MediaInformationParser substring:input from:@"'" to:@"'" ignoring:nil];
    
    return ^(NSString **s1, NSString **s2){
        *s1 = format;
        *s2 = path;
    };
}

+ (void (^)(NSNumber *__autoreleasing*, NSNumber *__autoreleasing*, NSNumber *__autoreleasing*))parseDurationBlock:(NSString*)input {
    NSNumber *duration = [MediaInformationParser parseDuration:[MediaInformationParser substring:input from:@"Duration:" to:@"," ignoring:[[NSMutableArray alloc] initWithObjects:@"uration:", nil]]];
    NSNumber *start = [MediaInformationParser parseStartTime:[MediaInformationParser substring:input from:@"start:" to:@"," ignoring:[[NSMutableArray alloc] initWithObjects:@"tart:", nil]]];
    NSString *bitrateString = [MediaInformationParser substring:input from:@"bitrate:" ignoring:[[NSMutableArray alloc] initWithObjects:@"itrate:", @"kb/s", nil]];

    NSNumber *bitrate = nil;
    if (bitrateString != nil && ![bitrateString isEqualToString:@"N/A"]) {
        bitrate = [MediaInformationParser toIntegerObject:bitrateString];
    }
    
    return ^(NSNumber **n1, NSNumber **n2, NSNumber **n3){
        *n1 = duration;
        *n2 = start;
        *n3 = bitrate;
    };
}

+ (void (^)(NSString *__autoreleasing*, NSString *__autoreleasing*))parseMetadataBlock:(NSString*)input {
    NSString *key = nil;
    NSString *value = nil;
    
    if (input != nil) {
        key = [MediaInformationParser substring:input to:@":" ignoring:[[NSMutableArray alloc] init]];
        value = [MediaInformationParser substring:input from:@":" ignoring:[[NSMutableArray alloc] init]];
    }
    
    return ^(NSString **s1, NSString **s2){
        *s1 = key;
        *s2 = value;
    };
}

+ (StreamInformation*)parseStreamBlock:(NSString*)input {
    StreamInformation* streamInformation = [[StreamInformation alloc] init];
    
    if (input != nil) {
        [streamInformation setIndex:[MediaInformationParser parseStreamIndex:input]];
        
        int typeBlockStartIndex = [MediaInformationParser index:input of:@":" from:0 times:2];
        if ((typeBlockStartIndex > -1) && (typeBlockStartIndex < [input length])) {
            
            NSString *streamBlock = [input substringFromIndex:(typeBlockStartIndex + 1)];
            
            NSArray* parts = [streamBlock componentsSeparatedByString:@","];
            NSString *typePart = [MediaInformationParser safeGet:parts from:0];
            NSString *type = [MediaInformationParser parseStreamType:typePart];
            
            [streamInformation setType:type];
            [streamInformation setCodec:[MediaInformationParser parseStreamCodec:typePart]];
            [streamInformation setFullCodec:[MediaInformationParser parseStreamFullCodec:typePart]];
            
            NSString *part2 = [MediaInformationParser safeGet:parts from:1];
            NSString *part3 = [MediaInformationParser safeGet:parts from:2];
            NSString *part4 = [MediaInformationParser safeGet:parts from:3];
            NSString *part5 = [MediaInformationParser safeGet:parts from:4];
            
            if ([@"video" isEqualToString:type]) {
                int lastUsedPart = 1;
                
                if (part2 != nil) {
                    int pStart = [MediaInformationParser count:part2 of:@"("];
                    int pEnd = [MediaInformationParser count:part2 of:@")"];

                    while (pStart != pEnd) {
                        lastUsedPart++;
                        NSString *newPart = [MediaInformationParser safeGet:parts from:lastUsedPart];
                        if (newPart == nil) {
                            break;
                        }
                        part2 = [NSString stringWithFormat:@"%@,%@", part2, newPart];
                        pStart = [MediaInformationParser count:part2 of:@"("];
                        pEnd = [MediaInformationParser count:part2 of:@")"];
                    }
                    
                    part2 = [part2 lowercaseString];
                    part2 = [part2 stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];

                    [streamInformation setFullFormat:part2];

                    NSString *formattedPart2 = [parenthesesRegex stringByReplacingMatchesInString:part2 options:0 range:NSMakeRange(0, [part2 length]) withTemplate:@""];
                    formattedPart2 = [formattedPart2 stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
                    [streamInformation setFormat:formattedPart2];
                }
                
                lastUsedPart++;
                NSString *videoDimensionPart = [MediaInformationParser safeGet:parts from:lastUsedPart];
                if (videoDimensionPart != nil) {
                    NSString *videoLayout = [videoDimensionPart lowercaseString];
                    videoLayout = [videoLayout stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];

                    void(^dimensions)(NSNumber **, NSNumber **) = [MediaInformationParser parseVideoDimensions:videoLayout];
                    NSNumber *width;
                    NSNumber *height;
                    dimensions(&width, &height);

                    [streamInformation setWidth:width];
                    [streamInformation setHeight:height];
                    
                    [streamInformation setSampleAspectRatio:[MediaInformationParser parseVideoStreamSampleAspectRatio:videoLayout]];
                    [streamInformation setDisplayAspectRatio:[MediaInformationParser parseVideoStreamDisplayAspectRatio:videoLayout]];
                }
                
                for (int i = lastUsedPart + 1; i < [parts count]; i++) {
                    NSString *part = [parts objectAtIndex:i];

                    part = [parenthesesRegex stringByReplacingMatchesInString:part options:0 range:NSMakeRange(0, [part length]) withTemplate:@""];
                    part = [part lowercaseString];
                    
                    if ([part rangeOfString:@"kb/s"].location != NSNotFound) {
                        part = [part stringByReplacingOccurrencesOfString:@"kb/s" withString:@""];
                        part = [part stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
                        NSNumber *bitrate = [MediaInformationParser toIntegerObject:part];
                        [streamInformation setBitrate:bitrate];
                    } else if ([part rangeOfString:@"fps"].location != NSNotFound) {
                        part = [part stringByReplacingOccurrencesOfString:@"fps" withString:@""];
                        part = [part stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
                        [streamInformation setAverageFrameRate:part];
                    } else if ([part rangeOfString:@"tbr"].location != NSNotFound) {
                        part = [part stringByReplacingOccurrencesOfString:@"tbr" withString:@""];
                        part = [part stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
                        [streamInformation setRealFrameRate:part];
                    } else if ([part rangeOfString:@"tbn"].location != NSNotFound) {
                        part = [part stringByReplacingOccurrencesOfString:@"tbn" withString:@""];
                        part = [part stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
                        [streamInformation setTimeBase:part];
                    } else if ([part rangeOfString:@"tbc"].location != NSNotFound) {
                        part = [part stringByReplacingOccurrencesOfString:@"tbc" withString:@""];
                        part = [part stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
                        [streamInformation setCodecTimeBase:part];
                    }
                }
            } else if ([@"audio" isEqualToString:type]) {
                if (part2 != nil) {
                    [streamInformation setSampleRate:[MediaInformationParser parseAudioStreamSampleRate:part2]];
                }
                if (part3 != nil) {
                    NSString *formattedPart3 = [part3 lowercaseString];
                    formattedPart3 = [formattedPart3 stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
                    [streamInformation setChannelLayout:formattedPart3];
                }
                if (part4 != nil) {
                    NSString *formattedPart4 = [part4 lowercaseString];
                    formattedPart4 = [formattedPart4 stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
                    [streamInformation setSampleFormat:formattedPart4];
                }
                if (part5 != nil) {
                    NSString *formattedPart5 = [part5 lowercaseString];
                    formattedPart5 = [parenthesesRegex stringByReplacingMatchesInString:formattedPart5 options:0 range:NSMakeRange(0, [formattedPart5 length]) withTemplate:@""];
                    formattedPart5 = [formattedPart5 stringByReplacingOccurrencesOfString:@"kb/s" withString:@""];
                    formattedPart5 = [formattedPart5 stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
                    [streamInformation setBitrate:[MediaInformationParser toIntegerObject:formattedPart5]];
                }
            } else if ([@"data" isEqualToString:type]) {
                if (part2 != nil) {
                    NSString *formattedPart2 = [part2 lowercaseString];
                    formattedPart2 = [parenthesesRegex stringByReplacingMatchesInString:formattedPart2 options:0 range:NSMakeRange(0, [formattedPart2 length]) withTemplate:@""];
                    formattedPart2 = [formattedPart2 stringByReplacingOccurrencesOfString:@"kb/s" withString:@""];
                    formattedPart2 = [formattedPart2 stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
                    [streamInformation setBitrate:[MediaInformationParser toIntegerObject:formattedPart2]];
                }
            }
        }
    }
    
    return streamInformation;
}

+ (void (^)(NSNumber *__autoreleasing*, NSNumber *__autoreleasing*))parseVideoDimensions: (NSString*)input {
    NSNumber *width = nil;
    NSNumber *height = nil;

    if (input != nil) {
        
        NSString *formattedString = [input lowercaseString];
        formattedString = [bracketsRegex stringByReplacingMatchesInString:formattedString options:0 range:NSMakeRange(0, [formattedString length]) withTemplate:@""];
        
        NSString *widthString = [MediaInformationParser substring:formattedString to:@"x" ignoring:[[NSMutableArray alloc] init]];
        NSString *heightString = [MediaInformationParser substring:formattedString from:@"x" ignoring:[[NSMutableArray alloc] init]];
        
        width = [MediaInformationParser toIntegerObject:widthString];
        height = [MediaInformationParser toIntegerObject:heightString];
    }
    
    return ^(NSNumber **n1, NSNumber **n2){
        *n1 = width;
        *n2 = height;
    };
}

+ (NSString*)parseVideoStreamSampleAspectRatio: (NSString*)input {
    if (input != nil) {

        NSString *formattedString = [input lowercaseString];
        formattedString = [formattedString stringByReplacingOccurrencesOfString:@"[" withString:@""];
        formattedString = [formattedString stringByReplacingOccurrencesOfString:@"]" withString:@""];
        
        NSArray* parts = [formattedString componentsSeparatedByString:@" "];
        
        for (int i=0; i < [parts count]; i++) {
            NSString *token = [parts objectAtIndex:i];
            if ([token isEqualToString:@"sar"]) {
                return [MediaInformationParser safeGet:parts from:(i + 1)];
            }
        }
    }
    
    return nil;
}

+ (NSString*)parseVideoStreamDisplayAspectRatio: (NSString*)input {
    if (input != nil) {
        
        NSString *formattedString = [input lowercaseString];
        formattedString = [formattedString stringByReplacingOccurrencesOfString:@"[" withString:@""];
        formattedString = [formattedString stringByReplacingOccurrencesOfString:@"]" withString:@""];
        
        NSArray* parts = [formattedString componentsSeparatedByString:@" "];
        
        for (int i=0; i < [parts count]; i++) {
            NSString *token = [parts objectAtIndex:i];
            if ([token isEqualToString:@"dar"]) {
                return [MediaInformationParser safeGet:parts from:(i + 1)];
            }
        }
    }
    
    return nil;
}

+ (NSNumber*)parseAudioStreamSampleRate: (NSString*)input {
    if (input != nil) {
        Boolean khz = false;
        Boolean mhz = false;
        NSString *lowerCaseString = [input lowercaseString];
        
        if ([lowerCaseString rangeOfString:@"khz"].location != NSNotFound) {
            khz = true;
        }
        if ([lowerCaseString rangeOfString:@"mhz"].location != NSNotFound) {
            mhz = true;
        }
        
        lowerCaseString = [lowerCaseString stringByReplacingOccurrencesOfString:@"khz" withString:@""];
        lowerCaseString = [lowerCaseString stringByReplacingOccurrencesOfString:@"mhz" withString:@""];
        lowerCaseString = [lowerCaseString stringByReplacingOccurrencesOfString:@"hz" withString:@""];

        NSNumber *sampleRate = [MediaInformationParser toInteger:[lowerCaseString stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]]];
        if (khz) {
            return [NSNumber numberWithInteger:(1000*sampleRate.intValue)];
        } else if (mhz) {
            return [NSNumber numberWithInteger:(1000000*sampleRate.intValue)];
        } else {
            return sampleRate;
        }
    }
    
    return nil;
}

+ (NSString*)parseStreamType: (NSString*)input {
    if (input != nil) {
        NSString *formattedString = [input lowercaseString];

        if ([formattedString rangeOfString:@"audio:"].location != NSNotFound) {
            return @"audio";
        } else if ([formattedString rangeOfString:@"video:"].location != NSNotFound) {
            return @"video";
        } else if ([formattedString rangeOfString:@"data:"].location != NSNotFound) {
            return @"data";
        }
    }
    
    return nil;
}

+ (NSString*)parseStreamCodec: (NSString*)input {
    if (input != nil) {
        NSString *formattedString = [input lowercaseString];
        formattedString = [parenthesesRegex stringByReplacingMatchesInString:formattedString options:0 range:NSMakeRange(0, [formattedString length]) withTemplate:@""];
        formattedString = [formattedString stringByReplacingOccurrencesOfString:@"video:" withString:@""];
        formattedString = [formattedString stringByReplacingOccurrencesOfString:@"audio:" withString:@""];
        formattedString = [formattedString stringByReplacingOccurrencesOfString:@"data:" withString:@""];

        return [formattedString stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
    }
    
    return nil;
}

+ (NSString*)parseStreamFullCodec: (NSString*)input {
    if (input != nil) {
        NSString *formattedString = [input lowercaseString];
        formattedString = [formattedString stringByReplacingOccurrencesOfString:@"video:" withString:@""];
        formattedString = [formattedString stringByReplacingOccurrencesOfString:@"audio:" withString:@""];
        formattedString = [formattedString stringByReplacingOccurrencesOfString:@"data:" withString:@""];

        return [formattedString stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
    }
    
    return nil;
}

+ (NSNumber*)parseStreamIndex: (NSString*)input {
    NSString *substring = [MediaInformationParser substring:input from:@"Stream #0:" to:@":" ignoring:[[NSMutableArray alloc] initWithObjects:@"tream #0", nil]];
    if (substring != nil) {
        substring = [substring stringByReplacingOccurrencesOfString:@":" withString:@""];
        substring = [parenthesesRegex stringByReplacingMatchesInString:substring options:0 range:NSMakeRange(0, [substring length]) withTemplate:@""];
        
        return [MediaInformationParser toIntegerObject:substring];
    }
    
    return nil;
}

+ (NSNumber*)parseDuration: (NSString*)input {
    if (input != nil && ![input isEqualToString:@"N/A"]) {
        NSString *seconds = [MediaInformationParser substring:input to:@"." ignoring:[[NSMutableArray alloc] init]];
        if (seconds != nil) {
            NSDate* durationDate = [durationFormat dateFromString: seconds];
            if (durationDate != nil) {
                NSTimeInterval secondsInMilliseconds = [durationDate timeIntervalSinceDate:referenceDuration]*1000;
                NSNumber *centiSeconds = [MediaInformationParser toInteger:[MediaInformationParser substring:input from:@"." ignoring:[[NSMutableArray alloc] init]]];
                
                secondsInMilliseconds += centiSeconds.intValue*10;
                
                return [NSNumber numberWithInteger:secondsInMilliseconds];
            }
        }
    }
    
    return nil;
}

+ (NSNumber*)parseStartTime: (NSString*)input {
    if (input != nil && ![input isEqualToString:@"N/A"] && ([input length]>0)) {
        double inputAsDouble = [input doubleValue];
        return [NSNumber numberWithInteger:ceil(inputAsDouble*1000)];
    }
    
    return nil;
}

+ (NSString*)substring:(NSString*)string from:(NSString*)start to:(NSString*)end ignoring:(NSArray*)ignoredTokens {
    NSString *extractedSubstring = nil;
    
    if (string != nil) {
        NSRange formatStart = [string rangeOfString:start];
        if (formatStart.location != NSNotFound && (formatStart.location + start.length < [string length])) {
            
            NSRange formatEnd = [string rangeOfString:end options:NSLiteralSearch range:NSMakeRange(formatStart.location + start.length, string.length - (formatStart.location + start.length + 1))];

            if (formatEnd.location != NSNotFound) {
                extractedSubstring = [string substringWithRange:NSMakeRange(formatStart.location + start.length, formatEnd.location - (formatStart.location + start.length))];
            }
        }
    }

    if ((ignoredTokens != nil) && (extractedSubstring != nil)) {
        for (int i=0; i < [ignoredTokens count]; i++) {
            NSString *token = [ignoredTokens objectAtIndex:i];
            extractedSubstring = [extractedSubstring stringByReplacingOccurrencesOfString:token withString:@""];
        }
    }
    
    return (extractedSubstring == nil) ? nil : [extractedSubstring stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
}

+ (NSString*)substring:(NSString*)string from:(NSString*)start ignoring:(NSArray*)ignoredTokens {
    NSString *extractedSubstring = nil;
    
    if (string != nil) {
        NSRange formatStart = [string rangeOfString:start];
        if (formatStart.location != NSNotFound) {
            extractedSubstring = [string substringWithRange:NSMakeRange(formatStart.location + start.length, string.length - (formatStart.location + start.length))];
        }
    }
    
    if ((ignoredTokens != nil) && (extractedSubstring != nil)) {
        for (int i=0; i < [ignoredTokens count]; i++) {
            NSString *token = [ignoredTokens objectAtIndex:i];
            extractedSubstring = [extractedSubstring stringByReplacingOccurrencesOfString:token withString:@""];
        }
    }
    
    return (extractedSubstring == nil) ? nil : [extractedSubstring stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
}

+ (NSString*)substring:(NSString*)string to:(NSString*)start ignoring:(NSArray*)ignoredTokens {
    NSString *extractedSubstring = nil;
    
    if (string != nil) {
        NSRange formatStart = [string rangeOfString:start];
        if (formatStart.location != NSNotFound) {
            extractedSubstring = [string substringWithRange:NSMakeRange(0, formatStart.location)];
        }
    }
    
    if ((ignoredTokens != nil) && (extractedSubstring != nil)) {
        for (int i=0; i < [ignoredTokens count]; i++) {
            NSString *token = [ignoredTokens objectAtIndex:i];
            extractedSubstring = [extractedSubstring stringByReplacingOccurrencesOfString:token withString:@""];
        }
    }
    
    return (extractedSubstring == nil) ? nil : [extractedSubstring stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
}

+ (int)index:(NSString*)string of:(NSString*)substring from:(int)startIndex times:(int)n {
    int count = 1;
    startIndex -= substring.length;
    
    while (count <= n) {
        unsigned long searchIndex = startIndex + (int)substring.length;
        NSRange currentRange = [string rangeOfString:substring options:NSLiteralSearch range:NSMakeRange(searchIndex, string.length - searchIndex)];
        if (currentRange.location != NSNotFound) {
            startIndex = (int)currentRange.location;
        }
        
        count++;
    }

    return startIndex;    
}

+ (int)count:(NSString*)string of:(NSString*)substring {
    int count = 0;
    int index = 0 - (int)substring.length;

    do {
        int searchIndex = index + (int)substring.length;
        NSRange currentRange = [string rangeOfString:substring options:NSLiteralSearch range:NSMakeRange(searchIndex, string.length - searchIndex)];
        if (currentRange.location != NSNotFound) {
            count++;
            index = (int)currentRange.location;
        } else {
            index = -1;
        }
    } while (index >= 0);
    
    return count;
}

+ (NSNumber*)toInteger: (NSString*)input {
    if (input == nil) {
        return [NSNumber numberWithInt:0];
    }
    
    return [NSNumber numberWithInteger:input.integerValue];
}

+ (NSNumber*)toIntegerObject: (NSString*)input {
    if (input == nil) {
        return nil;
    }
    
    return [NSNumber numberWithInteger:input.integerValue];
}

+ (NSString*)safeGet:(NSArray*)array from:(int)index {
    if (array == nil) {
        return nil;
    }
    
    unsigned long size = [array count];
    if (size > index) {
        return [array objectAtIndex:index];
    } else {
        return nil;
    }
}

@end
