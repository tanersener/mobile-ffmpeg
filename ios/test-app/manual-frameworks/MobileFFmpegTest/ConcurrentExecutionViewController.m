//
// ConcurrentExecutionViewController.m
//
// Copyright (c) 2020 Taner Sener
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
#import <mobileffmpeg/MobileFFmpeg.h>
#import <mobileffmpeg/MobileFFprobe.h>
#import <mobileffmpeg/FFmpegExecution.h>
#import "ConcurrentExecutionViewController.h"

@interface ConcurrentExecutionViewController ()

@property (strong, nonatomic) IBOutlet UITextView *outputText;
@property (strong, nonatomic) IBOutlet UILabel *header;
@property (strong, nonatomic) IBOutlet UIButton *encode1Button;
@property (strong, nonatomic) IBOutlet UIButton *encode2Button;
@property (strong, nonatomic) IBOutlet UIButton *encode3Button;
@property (strong, nonatomic) IBOutlet UIButton *cancel1Button;
@property (strong, nonatomic) IBOutlet UIButton *cancel2Button;
@property (strong, nonatomic) IBOutlet UIButton *cancel3Button;
@property (strong, nonatomic) IBOutlet UIButton *cancelAllButton;

@end

@implementation ConcurrentExecutionViewController {
    long executionId1;
    long executionId2;
    long executionId3;
}

- (void)viewDidLoad {
    [super viewDidLoad];

    // STYLE UPDATE
    [Util applyButtonStyle: self.encode1Button];
    [Util applyButtonStyle: self.encode2Button];
    [Util applyButtonStyle: self.encode3Button];
    [Util applyButtonStyle: self.cancel1Button];
    [Util applyButtonStyle: self.cancel2Button];
    [Util applyButtonStyle: self.cancel3Button];
    [Util applyButtonStyle: self.cancelAllButton];
    [Util applyOutputTextStyle: self.outputText];
    [Util applyHeaderStyle: self.header];

    dispatch_async(dispatch_get_main_queue(), ^{
        [self setActive];
    });
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}

- (void)logCallback:(long)executionId :(int)level :(NSString*)message {
    dispatch_async(dispatch_get_main_queue(), ^{
        [self appendOutput: [NSString stringWithFormat:@"%ld:%@", executionId, message]];
    });
}

- (void)executeCallback:(long)executionId :(int)returnCode {
    if (returnCode == RETURN_CODE_CANCEL) {
        NSLog(@"FFmpeg process ended with cancel with executionId %ld.\n", executionId);
    } else {
        NSLog(@"FFmpeg process ended with rc %d with executionId %ld.", returnCode, executionId);
    }
}

- (IBAction)encode1Clicked:(id)sender {
    [self encode:1];
}

- (IBAction)encode2Clicked:(id)sender {
    [self encode:2];
}

- (IBAction)encode3Clicked:(id)sender {
    [self encode:3];
}

- (IBAction)cancel1Button:(id)sender {
    [self cancel:1];
}

- (IBAction)cancel2Button:(id)sender {
    [self cancel:2];
}

- (IBAction)cancel3Button:(id)sender {
    [self cancel:3];
}

- (IBAction)cancelAllButton:(id)sender {
    [self cancel:0];
}

- (void)encode:(int)buttonNumber {
    [MobileFFmpegConfig setLogDelegate:self];

    NSString* docFolder = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
    NSString *resourceFolder = [[NSBundle mainBundle] resourcePath];
    NSString *image1 = [resourceFolder stringByAppendingPathComponent: @"colosseum.jpg"];
    NSString *image2 = [resourceFolder stringByAppendingPathComponent: @"pyramid.jpg"];
    NSString *image3 = [resourceFolder stringByAppendingPathComponent: @"tajmahal.jpg"];
    NSString *videoFile = [docFolder stringByAppendingPathComponent: [NSString stringWithFormat:@"video%d.mp4", buttonNumber]];

    NSLog(@"Testing CONCURRENT EXECUTION for button %d.\n", buttonNumber);

    NSString* ffmpegCommand = [ConcurrentExecutionViewController generateVideoEncodeScript:image1:image2:image3:videoFile:@"mpeg4":@""];

    NSLog(@"FFmpeg process started with arguments\n\'%@\'\n", ffmpegCommand);

    // EXECUTE
    long executionId = [MobileFFmpeg executeAsync:ffmpegCommand withCallback:self];

    NSLog(@"Async FFmpeg process started for button %d with executionId %ld.\n", buttonNumber, executionId);

    switch (buttonNumber) {
        case 1: {
            executionId1 = executionId;
        }
        break;
        case 2: {
            executionId2 = executionId;
        }
        break;
        default: {
            executionId3 = executionId;
        }
    }
    
    [self listFFmpegExecutions];
}

- (void)listFFmpegExecutions {
    NSArray* ffmpegExecutions = [MobileFFmpeg listExecutions];
    
    NSLog(@"Listing ongoing FFmpeg executions.\n");
    
    for (int i = 0; i < [ffmpegExecutions count]; i++) {
        FFmpegExecution* execution = [ffmpegExecutions objectAtIndex:i];
        NSLog(@"Execution %d= id: %ld, command: %@.\n", i, [execution getExecutionId], [execution getCommand]);
    }
    
    NSLog(@"Listed ongoing FFmpeg executions.\n");
}


- (void)cancel:(int)buttonNumber {
    long executionId = 0;

    switch (buttonNumber) {
        case 1: {
            executionId = executionId1;
        }
        break;
        case 2: {
            executionId = executionId2;
        }
        break;
        case 3: {
            executionId = executionId3;
        }
    }

    NSLog(@"Cancelling FFmpeg process for button %d with executionId %ld.\n", buttonNumber, executionId);

    if (executionId == 0) {
        [MobileFFmpeg cancel];
    } else {
        [MobileFFmpeg cancel:executionId];
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

+ (NSString*)generateVideoEncodeScript:(NSString *)image1 :(NSString *)image2 :(NSString *)image3 :(NSString *)videoFile :(NSString *)videoCodec :(NSString *)customOptions {
    return [NSString stringWithFormat:
@"-hide_banner -y -loop 1 -i %@ \
-loop 1 -i %@ \
-loop 1 -i %@ \
-filter_complex \"\
[0:v]setpts=PTS-STARTPTS,scale=w=\'if(gte(iw/ih,640/427),min(iw,640),-1)\':h=\'if(gte(iw/ih,640/427),-1,min(ih,427))\',scale=trunc(iw/2)*2:trunc(ih/2)*2,setsar=sar=1/1,split=2[stream1out1][stream1out2];\
[1:v]setpts=PTS-STARTPTS,scale=w=\'if(gte(iw/ih,640/427),min(iw,640),-1)\':h=\'if(gte(iw/ih,640/427),-1,min(ih,427))\',scale=trunc(iw/2)*2:trunc(ih/2)*2,setsar=sar=1/1,split=2[stream2out1][stream2out2];\
[2:v]setpts=PTS-STARTPTS,scale=w=\'if(gte(iw/ih,640/427),min(iw,640),-1)\':h=\'if(gte(iw/ih,640/427),-1,min(ih,427))\',scale=trunc(iw/2)*2:trunc(ih/2)*2,setsar=sar=1/1,split=2[stream3out1][stream3out2];\
[stream1out1]pad=width=640:height=427:x=(640-iw)/2:y=(427-ih)/2:color=#00000000,trim=duration=3,select=lte(n\\,90)[stream1overlaid];\
[stream1out2]pad=width=640:height=427:x=(640-iw)/2:y=(427-ih)/2:color=#00000000,trim=duration=1,select=lte(n\\,30),fade=t=out:s=0:n=30[stream1fadeout];\
[stream2out1]pad=width=640:height=427:x=(640-iw)/2:y=(427-ih)/2:color=#00000000,trim=duration=2,select=lte(n\\,60)[stream2overlaid];\
[stream2out2]pad=width=640:height=427:x=(640-iw)/2:y=(427-ih)/2:color=#00000000,trim=duration=1,select=lte(n\\,30),split=2[stream2starting][stream2ending];\
[stream3out1]pad=width=640:height=427:x=(640-iw)/2:y=(427-ih)/2:color=#00000000,trim=duration=2,select=lte(n\\,60)[stream3overlaid];\
[stream3out2]pad=width=640:height=427:x=(640-iw)/2:y=(427-ih)/2:color=#00000000,trim=duration=1,select=lte(n\\,30),fade=t=in:s=0:n=30[stream3fadein];\
[stream2starting]fade=t=in:s=0:n=30[stream2fadein];\
[stream2ending]fade=t=out:s=0:n=30[stream2fadeout];\
[stream2fadein][stream1fadeout]overlay=(main_w-overlay_w)/2:(main_h-overlay_h)/2,trim=duration=1,select=lte(n\\,30)[stream2blended];\
[stream3fadein][stream2fadeout]overlay=(main_w-overlay_w)/2:(main_h-overlay_h)/2,trim=duration=1,select=lte(n\\,30)[stream3blended];\
[stream1overlaid][stream2blended][stream2overlaid][stream3blended][stream3overlaid]concat=n=5:v=1:a=0,scale=w=640:h=424,format=yuv420p[video]\" \
-map [video] -vsync 2 -async 1 %@-c:v %@ -r 30 %@", image1, image2, image3, customOptions, videoCodec, videoFile];
}

@end
