//
// HttpsViewController.m
//
// Copyright (c) 2018-2019 Taner Sener
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

#import <mobileffmpeg/MobileFFmpegConfig.h>
#import <mobileffmpeg/MobileFFprobe.h>
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
    
    MediaInformation* information = [MobileFFprobe getMediaInformation:testUrl];

    if (information == nil) {
        NSLog(@"Get media information failed\n");
    } else {
        NSLog(@"Media information for %@\n", [information getFilename]);
        
        if ([information getFormat] != nil) {
            [self appendOutput:[NSString stringWithFormat:@"Format: %@\n", [information getFormat]]];
        }
        if ([information getBitrate] != nil) {
            [self appendOutput:[NSString stringWithFormat:@"Bitrate: %@\n", [information getBitrate]]];
        }
        if ([information getDuration] != nil) {
            [self appendOutput:[NSString stringWithFormat:@"Duration: %@\n", [information getDuration]]];
        }
        if ([information getStartTime] != nil) {
            [self appendOutput:[NSString stringWithFormat:@"Start time: %@\n", [information getStartTime]]];
        }
        if ([information getTags] != nil) {
            NSDictionary* tags = [information getTags];
            for(NSString *key in [tags allKeys]) {
                [self appendOutput:[NSString stringWithFormat:@"Tag: %@:%@", key, [tags objectForKey:key]]];
            }
        }
        if ([information getStreams] != nil) {
            for (StreamInformation* stream in [information getStreams]) {
                if ([stream getIndex] != nil) {
                    [self appendOutput:[NSString stringWithFormat:@"Stream index: %@\n", [stream getIndex]]];
                }
                if ([stream getType] != nil) {
                    [self appendOutput:[NSString stringWithFormat:@"Stream type: %@\n", [stream getType]]];
                }
                if ([stream getCodec] != nil) {
                    [self appendOutput:[NSString stringWithFormat:@"Stream codec: %@\n", [stream getCodec]]];
                }
                if ([stream getFullCodec] != nil) {
                    [self appendOutput:[NSString stringWithFormat:@"Stream full codec: %@\n", [stream getFullCodec]]];
                }
                if ([stream getFormat] != nil) {
                    [self appendOutput:[NSString stringWithFormat:@"Stream format: %@\n", [stream getFormat]]];
                }

                if ([stream getWidth] != nil) {
                    [self appendOutput:[NSString stringWithFormat:@"Stream width: %@\n", [stream getWidth]]];
                }
                if ([stream getHeight] != nil) {
                    [self appendOutput:[NSString stringWithFormat:@"Stream height: %@\n", [stream getHeight]]];
                }

                if ([stream getBitrate] != nil) {
                    [self appendOutput:[NSString stringWithFormat:@"Stream bitrate: %@\n", [stream getBitrate]]];
                }
                if ([stream getSampleRate] != nil) {
                    [self appendOutput:[NSString stringWithFormat:@"Stream sample rate: %@\n", [stream getSampleRate]]];
                }
                if ([stream getSampleFormat] != nil) {
                    [self appendOutput:[NSString stringWithFormat:@"Stream sample format: %@\n", [stream getSampleFormat]]];
                }
                if ([stream getChannelLayout] != nil) {
                    [self appendOutput:[NSString stringWithFormat:@"Stream channel layout: %@\n", [stream getChannelLayout]]];
                }

                if ([stream getSampleAspectRatio] != nil) {
                    [self appendOutput:[NSString stringWithFormat:@"Stream sample aspect ratio: %@\n", [stream getSampleAspectRatio]]];
                }
                if ([stream getDisplayAspectRatio] != nil) {
                    [self appendOutput:[NSString stringWithFormat:@"Stream display ascpect ratio: %@\n", [stream getDisplayAspectRatio]]];
                }
                if ([stream getAverageFrameRate] != nil) {
                    [self appendOutput:[NSString stringWithFormat:@"Stream average frame rate: %@\n", [stream getAverageFrameRate]]];
                }
                if ([stream getRealFrameRate] != nil) {
                    [self appendOutput:[NSString stringWithFormat:@"Stream real frame rate: %@\n", [stream getRealFrameRate]]];
                }
                if ([stream getTimeBase] != nil) {
                    [self appendOutput:[NSString stringWithFormat:@"Stream time base: %@\n", [stream getTimeBase]]];
                }
                if ([stream getCodecTimeBase] != nil) {
                    [self appendOutput:[NSString stringWithFormat:@"Stream codec time base: %@\n", [stream getCodecTimeBase]]];
                }

                if ([stream getTags] != nil) {
                    NSDictionary* tags = [stream getTags];
                    for(NSString *key in [tags allKeys]) {
                        [self appendOutput:[NSString stringWithFormat:@"Stream tag: %@:%@", key, [tags objectForKey:key]]];
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
