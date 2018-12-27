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

#include "fftools_ffmpeg.h"
#include "MobileFFmpegConfig.h"
#include "ArchDetect.h"
#include "MobileFFmpeg.h"

typedef enum {
    LogType = 1,
    StatisticsType = 2
} CallbackType;

/**
 * Callback data class.
 */
@interface CallbackData : NSObject

@end

@implementation CallbackData {

    CallbackType type;

    int logLevel;                       // log level
    NSString *logData;                  // log data

    int statisticsFrameNumber;          // statistics frame number
    float statisticsFps;                // statistics fps
    float statisticsQuality;            // statistics quality
    int64_t statisticsSize;             // statistics size
    int statisticsTime;                 // statistics time
    double statisticsBitrate;           // statistics bitrate
    double statisticsSpeed;             // statistics speed
}

 - (instancetype)initWithLogLevel:(int)newLogLevel data:(NSString*)newData {
    self = [super init];
    if (self) {
        type = LogType;

        logLevel = newLogLevel;
        logData = newData;
    }

    return self;
}

 - (instancetype)initWithVideoFrameNumber: (int)videoFrameNumber
                            fps:(float)videoFps
                            quality:(float)videoQuality
                            size:(int64_t)size
                            time:(int)time
                            bitrate:(double)bitrate
                            speed:(double)speed {
    self = [super init];
    if (self) {
        type = StatisticsType;

        statisticsFrameNumber = videoFrameNumber;
        statisticsFps = videoFps;
        statisticsQuality = videoQuality;
        statisticsSize = size;
        statisticsTime = time;
        statisticsBitrate = bitrate;
        statisticsSpeed = speed;
    }

    return self;
}

- (CallbackType)getType {
    return type;
}

- (int)getLogLevel {
    return logLevel;
}

- (NSString*)getLogData {
    return logData;
}

- (int)getStatisticsFrameNumber {
    return statisticsFrameNumber;
}

- (float)getStatisticsFps {
    return statisticsFps;
}

- (float)getStatisticsQuality {
    return statisticsQuality;
}

- (int64_t)getStatisticsSize {
    return statisticsSize;
}

- (int)getStatisticsTime {
    return statisticsTime;
}

- (double)getStatisticsBitrate {
    return statisticsBitrate;
}

- (double)getStatisticsSpeed {
    return statisticsSpeed;
}

@end

/** Redirection control variables */
static int redirectionEnabled;
static NSRecursiveLock *lock;
static dispatch_semaphore_t semaphore;
static NSMutableArray *callbackDataArray;

/** Holds delegate defined to redirect logs */
static id<LogDelegate> logDelegate = nil;

/** Holds delegate defined to redirect statistics */
static id<StatisticsDelegate> statisticsDelegate = nil;

NSString *const LIB_NAME = @"mobile-ffmpeg";

static Statistics *lastReceivedStatistics = nil;

static NSMutableArray *supportedExternalLibraries;

extern NSMutableString *lastCommandOutput;

NSMutableString *systemCommandOutput;
static int runningSystemCommand;

void callbackWait(int milliSeconds) {
    dispatch_semaphore_wait(semaphore, dispatch_time(DISPATCH_TIME_NOW, (int64_t)(milliSeconds * NSEC_PER_MSEC)));
}

void callbackNotify() {
    dispatch_semaphore_signal(semaphore);
}

/**
 * Adds log data to the end of callback data list.
 */
void logCallbackDataAdd(int level, NSString *logData) {
    CallbackData *callbackData = [[CallbackData alloc] initWithLogLevel:level data:logData];

    [lock lock];
    [callbackDataArray addObject:callbackData];
    [lock unlock];

    callbackNotify();
}

/**
 * Adds statistics data to the end of callback data list.
 */
void statisticsCallbackDataAdd(int frameNumber, float fps, float quality, int64_t size, int time, double bitrate, double speed) {
    CallbackData *callbackData = [[CallbackData alloc] initWithVideoFrameNumber:frameNumber fps:fps quality:quality size:size time:time bitrate:bitrate speed:speed];

    [lock lock];
    [callbackDataArray addObject:callbackData];
    [lock unlock];

    callbackNotify();
}

/**
 * Removes head of callback data list.
 */
CallbackData *callbackDataRemove() {
    CallbackData *newData = nil;

    [lock lock];

    @try {
        newData = [callbackDataArray objectAtIndex:0];
        [callbackDataArray removeObjectAtIndex:0];
    } @catch(NSException *exception) {
        // DO NOTHING
    } @finally {
        [lock unlock];
    }

    return newData;
}

/**
 * Callback function for FFmpeg logs.
 *
 * \param pointer to AVClass struct
 * \param level
 * \param format
 * \param arguments
 */
void mobileffmpeg_log_callback_function(void *ptr, int level, const char* format, va_list vargs) {

    // DO NOT PROCESS UNWANTED LOGS
    if (level >= 0) {
        level &= 0xff;
    }
    int activeLogLevel = [MobileFFmpegConfig getLogLevel];
    if ((activeLogLevel == AV_LOG_QUIET) || (level > activeLogLevel)) {
        return;
    }

    NSString *logData = [[NSString alloc] initWithFormat:[NSString stringWithCString:format encoding:NSUTF8StringEncoding] arguments:vargs];

    logCallbackDataAdd(level, logData);
}

/**
 * Callback function for FFmpeg statistics.
 *
 * \param frameNumber last processed frame number
 * \param fps frames processed per second
 * \param quality quality of the output stream (video only)
 * \param size size in bytes
 * \param time processed output duration
 * \param bitrate output bit rate in kbits/s
 * \param speed processing speed = processed duration / operation duration
 */
void mobileffmpeg_statistics_callback_function(int frameNumber, float fps, float quality, int64_t size, int time, double bitrate, double speed) {
    statisticsCallbackDataAdd(frameNumber, fps, quality, size, time, bitrate, speed);
}

/**
 * Forwards callback messages to Delegates.
 */
void callbackBlockFunction() {
    NSLog(@"Async callback block started.\n");

    while(redirectionEnabled) {
        @autoreleasepool {
            @try {

                CallbackData *callbackData = callbackDataRemove();
                if (callbackData != nil) {

                    if ([callbackData getType] == LogType) {

                        // LOG CALLBACK
                        int activeLogLevel = [MobileFFmpegConfig getLogLevel];

                        if (runningSystemCommand == 1) {

                            // REDIRECT SYSTEM OUTPUT
                            if ((activeLogLevel != AV_LOG_QUIET) && ([callbackData getLogLevel] <= activeLogLevel)) {
                                [systemCommandOutput appendString:[callbackData getLogData]];
                            }
                        } else if ((activeLogLevel == AV_LOG_QUIET) || ([callbackData getLogLevel] > activeLogLevel)) {

                            // LOG NEITHER PRINTED NOR FORWARDED
                        } else {

                            // ALWAYS REDIRECT COMMAND OUTPUT
                            [lastCommandOutput appendString:[callbackData getLogData]];

                            if (logDelegate != nil) {

                                // FORWARD LOG TO DELEGATE
                                [logDelegate logCallback:[callbackData getLogLevel]:[callbackData getLogData]];

                            } else {

                                // WRITE TO NSLOG
                                NSLog(@"%@: %@", [MobileFFmpegConfig logLevelToString:[callbackData getLogLevel]], [callbackData getLogData]);
                            }

                        }

                    } else {

                        // STATISTICS CALLBACK
                        Statistics *newStatistics = [[Statistics alloc] initWithVideoFrameNumber:[callbackData getStatisticsFrameNumber] fps:[callbackData getStatisticsFps] quality:[callbackData getStatisticsQuality] size:[callbackData getStatisticsSize] time:[callbackData getStatisticsTime] bitrate:[callbackData getStatisticsBitrate] speed:[callbackData getStatisticsSpeed]];
                        [lastReceivedStatistics update:newStatistics];

                        if (logDelegate != nil) {

                            // FORWARD STATISTICS TO DELEGATE
                            [statisticsDelegate statisticsCallback:lastReceivedStatistics];
                        }
                    }

                } else {
                    callbackWait(100);
                }

            } @catch(NSException *exception) {
                NSLog(@"Async callback block received error: %@n\n", exception);
                NSLog(@"%@", [exception callStackSymbols]);
            }
        }
    }

    NSLog(@"Async callback block stopped.\n");
}

static int systemCommandOutputContainsPattern(NSArray *patternList) {
    for (int i=0; i < [patternList count]; i++) {
        NSString *pattern = [patternList objectAtIndex:i];
        if ([systemCommandOutput rangeOfString:pattern].location != NSNotFound) {
            return 1;
        }
    }

    return 0;
}

/**
 * Executes system command. System command is not logged to output.
 *
 * \param arguments command arguments
 * \param commandOutputEndPatternList list of patterns which will indicate that operation has ended
 * \param timeout execution timeout
 * \return return code
 */
int mobileffmpeg_system_execute(NSArray *arguments, NSArray *commandOutputEndPatternList, long timeout) {
    systemCommandOutput = [[NSMutableString alloc] init];
    runningSystemCommand = 1;

    int rc = [MobileFFmpeg executeWithArguments:arguments];

    long totalWaitTime = 0;

    while ((systemCommandOutputContainsPattern(commandOutputEndPatternList) == 0) && (totalWaitTime < timeout)) {
        [NSThread sleepForTimeInterval:.02];
        totalWaitTime += 20;
    }

    runningSystemCommand = 0;

    [MobileFFmpeg cancel];

    return rc;
}

@interface MobileFFmpegConfig()

/**
 * Returns build configuration for FFmpeg.
 *
 * \return build configuration string
 */
+ (NSString*)getBuildConf;

@end

@implementation MobileFFmpegConfig

+ (void)initialize {
    supportedExternalLibraries = [[NSMutableArray alloc] init];
    [supportedExternalLibraries addObject:@"fontconfig"];
    [supportedExternalLibraries addObject:@"freetype"];
    [supportedExternalLibraries addObject:@"fribidi"];
    [supportedExternalLibraries addObject:@"gmp"];
    [supportedExternalLibraries addObject:@"gnutls"];
    [supportedExternalLibraries addObject:@"kvazaar"];
    [supportedExternalLibraries addObject:@"mp3lame"];
    [supportedExternalLibraries addObject:@"libaom"];
    [supportedExternalLibraries addObject:@"libass"];
    [supportedExternalLibraries addObject:@"iconv"];
    [supportedExternalLibraries addObject:@"libilbc"];
    [supportedExternalLibraries addObject:@"libtheora"];
    [supportedExternalLibraries addObject:@"libvidstab"];
    [supportedExternalLibraries addObject:@"libvorbis"];
    [supportedExternalLibraries addObject:@"libvpx"];
    [supportedExternalLibraries addObject:@"libwebp"];
    [supportedExternalLibraries addObject:@"libxml2"];
    [supportedExternalLibraries addObject:@"opencore-amr"];
    [supportedExternalLibraries addObject:@"opus"];
    [supportedExternalLibraries addObject:@"shine"];
    [supportedExternalLibraries addObject:@"snappy"];
    [supportedExternalLibraries addObject:@"soxr"];
    [supportedExternalLibraries addObject:@"speex"];
    [supportedExternalLibraries addObject:@"twolame"];
    [supportedExternalLibraries addObject:@"wavpack"];
    [supportedExternalLibraries addObject:@"x264"];
    [supportedExternalLibraries addObject:@"x265"];
    [supportedExternalLibraries addObject:@"xvid"];

    [ArchDetect class];
    [MobileFFmpeg class];

    redirectionEnabled = 0;
    lock = [[NSRecursiveLock alloc] init];
    semaphore = dispatch_semaphore_create(0);
    lastReceivedStatistics = [[Statistics alloc] init];
    callbackDataArray = [[NSMutableArray alloc] init];

    runningSystemCommand = 0;
    systemCommandOutput = [[NSMutableString alloc] init];

    [MobileFFmpegConfig enableRedirection];
}

/**
 * Enables log and statistics redirection.
 */
+ (void)enableRedirection {
    [lock lock];

    if (redirectionEnabled != 0) {
        [lock unlock];
        return;
    }
    redirectionEnabled = 1;

    [lock unlock];

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        callbackBlockFunction();
    });

    av_log_set_callback(mobileffmpeg_log_callback_function);
    set_report_callback(mobileffmpeg_statistics_callback_function);
}

/**
 * Disables log and statistics redirection.
 */
+ (void)disableRedirection {
    [lock lock];

    if (redirectionEnabled != 1) {
        [lock unlock];
        return;
    }
    redirectionEnabled = 0;

    [lock unlock];

    av_log_set_callback(av_log_default_callback);
    set_report_callback(nil);

    callbackNotify();
}

/**
 * Returns log level.
 *
 * \return log level
 */
+ (int)getLogLevel {
    return av_log_get_level();
}

/**
 * Sets log level.
 *
 * \param log level
 */
+ (void)setLogLevel: (int)level {
    av_log_set_level(level);
}

/**
 * Converts int log level to string.
 *
 * \param level value
 * \return string value
 */
+ (NSString*)logLevelToString: (int)level {
    switch (level) {
        case AV_LOG_TRACE: return @"TRACE";
        case AV_LOG_DEBUG: return @"DEBUG";
        case AV_LOG_VERBOSE: return @"VERBOSE";
        case AV_LOG_INFO: return @"INFO";
        case AV_LOG_WARNING: return @"WARNING";
        case AV_LOG_ERROR: return @"ERROR";
        case AV_LOG_FATAL: return @"FATAL";
        case AV_LOG_PANIC: return @"PANIC";
        case AV_LOG_QUIET:
        default: return @"";
    }
}

/**
 * Sets a LogDelegate. logCallback method inside LogDelegate is used to redirect logs.
 *
 * \param new log delegate
 */
+ (void)setLogDelegate: (id<LogDelegate>)newLogDelegate {
    logDelegate = newLogDelegate;
}

/**
 * Sets a StatisticsDelegate.
 *
 * \param statistics delegate
 */
+ (void)setStatisticsDelegate: (id<StatisticsDelegate>)newStatisticsDelegate {
    statisticsDelegate = newStatisticsDelegate;
}

/**
 * Returns the last received statistics data.
 *
 * \return last received statistics data
 */
+ (Statistics*)getLastReceivedStatistics {
    return lastReceivedStatistics;
}

/**
 * Resets last received statistics.
 */
+ (void)resetStatistics {
    lastReceivedStatistics = [[Statistics alloc] init];
}

/**
 * Sets and overrides fontconfig configuration directory.
 *
 * \param directory which contains fontconfig configuration (fonts.conf)
 */
+ (void)setFontconfigConfigurationPath: (NSString*)path {
    if (path != nil) {
        setenv("FONTCONFIG_PATH", [path UTF8String], true);
    }
}

/**
 * Registers fonts inside the given path, so they are available in FFmpeg filters.
 *
 * Note that you need to build MobileFFmpeg with fontconfig
 * enabled or use a prebuilt package with fontconfig inside to use this feature.
 *
 * \param directory which contains fonts (.ttf and .otf files)
 * \param custom font name mappings, useful to access your fonts with more friendly names
 */
+ (void)setFontDirectory: (NSString*)fontDirectoryPath with:(NSDictionary*)fontNameMapping {
    NSError *error = nil;
    BOOL isDirectory = YES;
    BOOL isFile = NO;
    int validFontNameMappingCount = 0;
    NSString *tempConfigurationDirectory = [NSTemporaryDirectory() stringByAppendingPathComponent:@".mobileffmpeg"];
    NSString *fontConfigurationFile = [tempConfigurationDirectory stringByAppendingPathComponent:@"fonts.conf"];

    if (![[NSFileManager defaultManager] fileExistsAtPath:tempConfigurationDirectory isDirectory:&isDirectory]) {

        if (![[NSFileManager defaultManager] createDirectoryAtPath:tempConfigurationDirectory withIntermediateDirectories:YES attributes:nil error:&error]) {
            NSLog(@"Failed to set font directory. Error received while creating temp conf directory: %@.", error);
            return;
        }

        NSLog(@"Created temporary font conf directory: TRUE.");
    }

    if ([[NSFileManager defaultManager] fileExistsAtPath:fontConfigurationFile isDirectory:&isFile]) {
        BOOL fontConfigurationDeleted = [[NSFileManager defaultManager] removeItemAtPath:fontConfigurationFile error:NULL];
        NSLog(@"Deleted old temporary font configuration: %s.", fontConfigurationDeleted?"TRUE":"FALSE");
    }

    /* PROCESS MAPPINGS FIRST */
    NSString *fontNameMappingBlock = @"";
    for (NSString *fontName in [fontNameMapping allKeys]) {
        NSString *mappedFontName = [fontNameMapping objectForKey:fontName];

        if ((fontName != nil) && (mappedFontName != nil) && ([fontName length] > 0) && ([mappedFontName length] > 0)) {

            fontNameMappingBlock = [NSString stringWithFormat:@"%@\n%@\n%@%@%@\n%@\n%@\n%@%@%@\n%@\n%@\n",
                @"        <match target=\"pattern\">",
                @"                <test qual=\"any\" name=\"family\">",
                @"                        <string>", fontName, @"</string>",
                @"                </test>",
                @"                <edit name=\"family\" mode=\"assign\" binding=\"same\">",
                @"                        <string>", mappedFontName, @"</string>",
                @"                </edit>",
                @"        </match>"];

            validFontNameMappingCount++;
        }
    }

    NSString *fontConfiguration = [NSString stringWithFormat:@"%@\n%@\n%@\n%@\n%@%@%@\n%@\n%@\n",
                            @"<?xml version=\"1.0\"?>",
                            @"<!DOCTYPE fontconfig SYSTEM \"fonts.dtd\">",
                            @"<fontconfig>",
                            @"    <dir>.</dir>",
                            @"    <dir>", fontDirectoryPath, @"</dir>",
                            fontNameMappingBlock,
                            @"</fontconfig>"];

    if (![fontConfiguration writeToFile:fontConfigurationFile atomically:YES encoding:NSUTF8StringEncoding error:&error]) {
        NSLog(@"Failed to set font directory. Error received while saving font configuration: %@.", error);
        return;
    }
    NSLog(@"Saved new temporary font configuration with %d font name mappings.", validFontNameMappingCount);

    [MobileFFmpegConfig setFontconfigConfigurationPath:tempConfigurationDirectory];

    NSLog(@"Font directory %@ registered successfully.", fontDirectoryPath);
}

/**
 * Returns build configuration for FFmpeg.
 *
 * \return build configuration string
 */
+ (NSString*)getBuildConf {
    return [NSString stringWithUTF8String:FFMPEG_CONFIGURATION];
}

/**
 * Returns package name.
 *
 * \return guessed package name according to supported external libraries
 */
+ (NSString*)getPackageName {
    NSArray *enabledLibraryArray = [MobileFFmpegConfig getExternalLibraries];
    Boolean speex = [enabledLibraryArray containsObject:@"speex"];
    Boolean fribidi = [enabledLibraryArray containsObject:@"fribidi"];
    Boolean gnutls = [enabledLibraryArray containsObject:@"gnutls"];
    Boolean xvid = [enabledLibraryArray containsObject:@"xvid"];

    Boolean min = false;
    Boolean minGpl = false;
    Boolean https = false;
    Boolean httpsGpl = false;
    Boolean audio = false;
    Boolean video = false;
    Boolean full = false;
    Boolean fullGpl = false;

    if (speex && fribidi) {
        if (xvid) {
            fullGpl = true;
        } else {
            full = true;
        }
    } else if (speex) {
        audio = true;
    } else if (fribidi) {
        video = true;
    } else if (xvid) {
        if (gnutls) {
            httpsGpl = true;
        } else {
            minGpl = true;
        }
    } else {
        if (gnutls) {
            https = true;
        } else {
            min = true;
        }
    }

    if (fullGpl) {
        if ([enabledLibraryArray containsObject:@"fontconfig"] &&
            [enabledLibraryArray containsObject:@"freetype"] &&
            [enabledLibraryArray containsObject:@"fribidi"] &&
            [enabledLibraryArray containsObject:@"gmp"] &&
            [enabledLibraryArray containsObject:@"gnutls"] &&
            [enabledLibraryArray containsObject:@"kvazaar"] &&
            [enabledLibraryArray containsObject:@"mp3lame"] &&
            [enabledLibraryArray containsObject:@"libaom"] &&
            [enabledLibraryArray containsObject:@"libass"] &&
            [enabledLibraryArray containsObject:@"iconv"] &&
            [enabledLibraryArray containsObject:@"libilbc"] &&
            [enabledLibraryArray containsObject:@"libtheora"] &&
            [enabledLibraryArray containsObject:@"libvidstab"] &&
            [enabledLibraryArray containsObject:@"libvorbis"] &&
            [enabledLibraryArray containsObject:@"libvpx"] &&
            [enabledLibraryArray containsObject:@"libwebp"] &&
            [enabledLibraryArray containsObject:@"libxml2"] &&
            [enabledLibraryArray containsObject:@"opencore-amr"] &&
            [enabledLibraryArray containsObject:@"opus"] &&
            [enabledLibraryArray containsObject:@"shine"] &&
            [enabledLibraryArray containsObject:@"snappy"] &&
            [enabledLibraryArray containsObject:@"soxr"] &&
            [enabledLibraryArray containsObject:@"speex"] &&
            [enabledLibraryArray containsObject:@"twolame"] &&
            [enabledLibraryArray containsObject:@"wavpack"] &&
            [enabledLibraryArray containsObject:@"x264"] &&
            [enabledLibraryArray containsObject:@"x265"] &&
            [enabledLibraryArray containsObject:@"xvid"]) {
            return @"full-gpl";
        } else {
            return @"custom";
        }
    }

    if (full) {
        if ([enabledLibraryArray containsObject:@"fontconfig"] &&
            [enabledLibraryArray containsObject:@"freetype"] &&
            [enabledLibraryArray containsObject:@"fribidi"] &&
            [enabledLibraryArray containsObject:@"gmp"] &&
            [enabledLibraryArray containsObject:@"gnutls"] &&
            [enabledLibraryArray containsObject:@"kvazaar"] &&
            [enabledLibraryArray containsObject:@"mp3lame"] &&
            [enabledLibraryArray containsObject:@"libaom"] &&
            [enabledLibraryArray containsObject:@"libass"] &&
            [enabledLibraryArray containsObject:@"iconv"] &&
            [enabledLibraryArray containsObject:@"libilbc"] &&
            [enabledLibraryArray containsObject:@"libtheora"] &&
            [enabledLibraryArray containsObject:@"libvorbis"] &&
            [enabledLibraryArray containsObject:@"libvpx"] &&
            [enabledLibraryArray containsObject:@"libwebp"] &&
            [enabledLibraryArray containsObject:@"libxml2"] &&
            [enabledLibraryArray containsObject:@"opencore-amr"] &&
            [enabledLibraryArray containsObject:@"opus"] &&
            [enabledLibraryArray containsObject:@"shine"] &&
            [enabledLibraryArray containsObject:@"snappy"] &&
            [enabledLibraryArray containsObject:@"soxr"] &&
            [enabledLibraryArray containsObject:@"speex"] &&
            [enabledLibraryArray containsObject:@"twolame"] &&
            [enabledLibraryArray containsObject:@"wavpack"]) {
            return @"full";
        } else {
            return @"custom";
        }
    }

    if (video) {
        if ([enabledLibraryArray containsObject:@"fontconfig"] &&
            [enabledLibraryArray containsObject:@"freetype"] &&
            [enabledLibraryArray containsObject:@"fribidi"] &&
            [enabledLibraryArray containsObject:@"kvazaar"] &&
            [enabledLibraryArray containsObject:@"libaom"] &&
            [enabledLibraryArray containsObject:@"libass"] &&
            [enabledLibraryArray containsObject:@"iconv"] &&
            [enabledLibraryArray containsObject:@"libtheora"] &&
            [enabledLibraryArray containsObject:@"libvpx"] &&
            [enabledLibraryArray containsObject:@"libwebp"] &&
            [enabledLibraryArray containsObject:@"snappy"]) {
            return @"video";
        } else {
            return @"custom";
        }
    }

    if (audio) {
        if ([enabledLibraryArray containsObject:@"mp3lame"] &&
            [enabledLibraryArray containsObject:@"libilbc"] &&
            [enabledLibraryArray containsObject:@"libvorbis"] &&
            [enabledLibraryArray containsObject:@"opencore-amr"] &&
            [enabledLibraryArray containsObject:@"opus"] &&
            [enabledLibraryArray containsObject:@"shine"] &&
            [enabledLibraryArray containsObject:@"soxr"] &&
            [enabledLibraryArray containsObject:@"speex"] &&
            [enabledLibraryArray containsObject:@"twolame"] &&
            [enabledLibraryArray containsObject:@"wavpack"]) {
            return @"audio";
        } else {
            return @"custom";
        }
    }

    if (httpsGpl) {
        if ([enabledLibraryArray containsObject:@"gmp"] &&
            [enabledLibraryArray containsObject:@"gnutls"] &&
            [enabledLibraryArray containsObject:@"libvidstab"] &&
            [enabledLibraryArray containsObject:@"x264"] &&
            [enabledLibraryArray containsObject:@"x265"] &&
            [enabledLibraryArray containsObject:@"xvid"]) {
            return @"https-gpl";
        } else {
            return @"custom";
        }
    }

    if (https) {
        if ([enabledLibraryArray containsObject:@"gmp"] &&
            [enabledLibraryArray containsObject:@"gnutls"]) {
            return @"https";
        } else {
            return @"custom";
        }
    }

    if (minGpl) {
        if ([enabledLibraryArray containsObject:@"libvidstab"] &&
            [enabledLibraryArray containsObject:@"x264"] &&
            [enabledLibraryArray containsObject:@"x265"] &&
            [enabledLibraryArray containsObject:@"xvid"]) {
            return @"min-gpl";
        } else {
            return @"custom";
        }
    }

    return @"min";
}

/**
 * Returns supported external libraries.
 *
 * \return array of supported external libraries
 */
+ (NSArray*)getExternalLibraries {
    NSString *buildConfiguration = [MobileFFmpegConfig getBuildConf];
    NSMutableArray *enabledLibraryArray = [[NSMutableArray alloc] init];

    for (int i=0; i < [supportedExternalLibraries count]; i++) {
        NSString *supportedExternalLibrary = [supportedExternalLibraries objectAtIndex:i];

        NSString *libraryName1 = [NSString stringWithFormat:@"enable-%@", supportedExternalLibrary];
        NSString *libraryName2 = [NSString stringWithFormat:@"enable-lib%@", supportedExternalLibrary];

        if ([buildConfiguration rangeOfString:libraryName1].location != NSNotFound || [buildConfiguration rangeOfString:libraryName2].location != NSNotFound) {
            [enabledLibraryArray addObject:supportedExternalLibrary];
        }
    }

    [enabledLibraryArray sortUsingSelector:@selector(compare:)];

    return enabledLibraryArray;
}

@end
