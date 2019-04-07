//
// HttpsViewController.m
//
// Copyright (c) 2018 Taner Sener
//
// This file is part of MobileFFmpeg.
//
// MobileFFmpeg is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// MobileFFmpeg is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with MobileFFmpeg.  If not, see <http://www.gnu.org/licenses/>.
//

#import <MobileFFmpegConfig.h>
#import <MobileFFmpeg.h>
#import "HttpsViewController.h"

@interface HttpsViewController ()

@property (strong, nonatomic) IBOutlet UILabel *header;
@property (strong, nonatomic) IBOutlet UITextField *urlText;
@property (strong, nonatomic) IBOutlet UIButton *getInfoButton;
@property (strong, nonatomic) IBOutlet UITextView *outputText;

@end

@implementation HttpsViewController {
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    // STYLE UPDATE
    [Util applyEditTextStyle: self.urlText];
    [Util applyButtonStyle: self.getInfoButton];
    [Util applyOutputTextStyle: self.outputText];
    [Util applyHeaderStyle: self.header];

    dispatch_async(dispatch_get_main_queue(), ^{
        [self setActive];
    });
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}

- (void)logCallback: (int)level :(NSString*)message {
    dispatch_async(dispatch_get_main_queue(), ^{
        [self appendOutput: message];
    });
}

- (IBAction)getInfoClicked:(id)sender {
    [self clearOutput];
    
    NSString *testUrl = [self.urlText text];
    if ([testUrl length] > 0) {
        NSLog(@"Testing HTTPS with url \'%@\'\n", testUrl);
    } else {
        testUrl = HTTPS_TEST_DEFAULT_URL;
        [self.urlText setText:testUrl];
        NSLog(@"Testing HTTPS with default url \'%@\'\n", testUrl);
    }
    
    MediaInformation* information = [MobileFFmpeg getMediaInformation:testUrl];

    if (information == nil) {
        NSLog(@"Get media information failed\n");
    } else {
        NSLog(@"Media information for %@\n", [information getPath]);
        
        if ([information getFormat] != nil) {
            NSLog(@"Format: %@\n", [information getFormat]);
        }
        if ([information getBitrate] != nil) {
            NSLog(@"Bitrate: %@\n", [information getBitrate]);
        }
        if ([information getDuration] != nil) {
            NSLog(@"Duration: %@\n", [information getDuration]);
        }
        if ([information getStartTime] != nil) {
            NSLog(@"Start time: %@\n", [information getStartTime]);
        }
        if ([information getMetadataEntries] != nil) {
            NSDictionary* entries = [information getMetadataEntries];
            for(NSString *key in [entries allKeys]) {
                NSLog(@"Metadata: %@:%@", key, [entries objectForKey:key]);
            }
        }
        if ([information getStreams] != nil) {
            for (StreamInformation* stream in [information getStreams]) {
                if ([stream getIndex] != nil) {
                    NSLog(@"Stream index: %@\n", [stream getIndex]);
                }
                if ([stream getType] != nil) {
                    NSLog(@"Stream type: %@\n", [stream getType]);
                }
                if ([stream getCodec] != nil) {
                    NSLog(@"Stream codec: %@\n", [stream getCodec]);
                }
                if ([stream getFullCodec] != nil) {
                    NSLog(@"Stream full codec: %@\n", [stream getFullCodec]);
                }
                if ([stream getFormat] != nil) {
                    NSLog(@"Stream format: %@\n", [stream getFormat]);
                }
                if ([stream getFullFormat] != nil) {
                    NSLog(@"Stream full format: %@\n", [stream getFullFormat]);
                }

                if ([stream getWidth] != nil) {
                    NSLog(@"Stream width: %@\n", [stream getWidth]);
                }
                if ([stream getHeight] != nil) {
                    NSLog(@"Stream height: %@\n", [stream getHeight]);
                }

                if ([stream getBitrate] != nil) {
                    NSLog(@"Stream bitrate: %@\n", [stream getBitrate]);
                }
                if ([stream getSampleRate] != nil) {
                    NSLog(@"Stream sample rate: %@\n", [stream getSampleRate]);
                }
                if ([stream getSampleFormat] != nil) {
                    NSLog(@"Stream sample format: %@\n", [stream getSampleFormat]);
                }
                if ([stream getChannelLayout] != nil) {
                    NSLog(@"Stream channel layout: %@\n", [stream getChannelLayout]);
                }

                if ([stream getSampleAspectRatio] != nil) {
                    NSLog(@"Stream sample aspect ratio: %@\n", [stream getSampleAspectRatio]);
                }
                if ([stream getDisplayAspectRatio] != nil) {
                    NSLog(@"Stream display ascpect ratio: %@\n", [stream getDisplayAspectRatio]);
                }
                if ([stream getAverageFrameRate] != nil) {
                    NSLog(@"Stream average frame rate: %@\n", [stream getAverageFrameRate]);
                }
                if ([stream getRealFrameRate] != nil) {
                    NSLog(@"Stream real frame rate: %@\n", [stream getRealFrameRate]);
                }
                if ([stream getTimeBase] != nil) {
                    NSLog(@"Stream time base: %@\n", [stream getTimeBase]);
                }
                if ([stream getCodecTimeBase] != nil) {
                    NSLog(@"Stream codec time base: %@\n", [stream getCodecTimeBase]);
                }

                if ([stream getMetadataEntries] != nil) {
                    NSDictionary* entries = [information getMetadataEntries];
                    for(NSString *key in [entries allKeys]) {
                        NSLog(@"Stream metadata: %@:%@", key, [entries objectForKey:key]);
                    }
                }
            }
        }
    }
}

- (void)setActive {
    [MobileFFmpegConfig setLogDelegate:self];
}

- (void)clearOutput {
    [[self outputText] setText:@""];
}

- (void)appendOutput:(NSString*) message {
    self.outputText.text = [self.outputText.text stringByAppendingString:message];
    
    if (self.outputText.text.length > 0 ) {
        NSRange bottom = NSMakeRange(self.outputText.text.length - 1, 1);
        [self.outputText scrollRangeToVisible:bottom];
    }
}

@end
