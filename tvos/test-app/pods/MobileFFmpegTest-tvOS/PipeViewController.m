//
// PipeViewController.m
//
// Copyright (c) 2019 Taner Sener
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

#import <AVFoundation/AVFoundation.h>
#import <AVKit/AVKit.h>
#import <mobileffmpeg/MobileFFmpegConfig.h>
#import <mobileffmpeg/MobileFFmpeg.h>
#import "PipeViewController.h"

@interface PipeViewController ()

@property (strong, nonatomic) IBOutlet UILabel *header;
@property (weak, nonatomic) IBOutlet UIButton *createButton;
@property (strong, nonatomic) IBOutlet UILabel *videoPlayerFrame;

@end

@implementation PipeViewController {

    // Video player references
    AVQueuePlayer *player;
    AVPlayerLayer *playerLayer;
    AVPlayerItem *activeItem;

    // Loading view
    UIAlertController *alertController;
    UIActivityIndicatorView* indicator;

    Statistics *statistics;
}

- (void)viewDidLoad {
    [super viewDidLoad];

    // STYLE UPDATE
    [Util applyButtonStyle: self.createButton];
    [Util applyVideoPlayerFrameStyle: self.videoPlayerFrame];
    [Util applyHeaderStyle: self.header];

    // VIDEO PLAYER INIT
    player = [[AVQueuePlayer alloc] init];
    playerLayer = [AVPlayerLayer playerLayerWithPlayer:player];
    playerLayer.videoGravity = AVLayerVideoGravityResize;
    activeItem = nil;

    // SETTING VIDEO FRAME POSITION
    CGRect rectangularFrame = CGRectMake(self.videoPlayerFrame.frame.origin.x + 20,
                                         self.videoPlayerFrame.frame.origin.y + 20,
                                         self.videoPlayerFrame.frame.size.width - 40,
                                         self.videoPlayerFrame.frame.size.height - 40);

    playerLayer.frame = rectangularFrame;
    [self.view.layer addSublayer:playerLayer];

    alertController = nil;
    statistics = nil;

    dispatch_async(dispatch_get_main_queue(), ^{
        [self setActive];
    });
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}

- (void)logCallback:(long)executionId :(int)level :(NSString*)message {
    dispatch_async(dispatch_get_main_queue(), ^{
        NSLog(@"%@", message);
    });
}

- (void)statisticsCallback:(Statistics *)newStatistics {
    dispatch_async(dispatch_get_main_queue(), ^{
        self->statistics = newStatistics;
        [self updateProgressDialog];
    });
}

+ (void)startAsyncCopyImageProcess: (NSString*)imagePath onPipe:(NSString*)namedPipePath {
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{

        NSLog(@"Starting copy %@ to pipe %@ operation.\n", imagePath, namedPipePath);

        NSFileHandle *fileHandle = [NSFileHandle fileHandleForReadingAtPath: imagePath];
        if (fileHandle == nil) {
            NSLog(@"Failed to open file %@.\n", imagePath);
            return;
        }

        NSFileHandle *pipeHandle = [NSFileHandle fileHandleForWritingAtPath: namedPipePath];
        if (pipeHandle == nil) {
            NSLog(@"Failed to open pipe %@.\n", namedPipePath);
            [fileHandle closeFile];
            return;
        }

        int BUFFER_SIZE = 4096;
        unsigned long readBytes = 0;
        unsigned long totalBytes = 0;
        double startTime = CACurrentMediaTime();

        @try {
            [fileHandle seekToFileOffset: 0];

            do {
                NSData *data = [fileHandle readDataOfLength:BUFFER_SIZE];
                readBytes = [data length];
                if (readBytes > 0) {
                    totalBytes += readBytes;
                    [pipeHandle writeData:data];
                }
            } while (readBytes > 0);

            double endTime = CACurrentMediaTime();

            NSLog(@"Completed copy %@ to pipe %@ operation. %lu bytes copied in %f seconds.\n", imagePath, namedPipePath, totalBytes, (endTime - startTime));

        } @catch (NSException *e) {
            NSLog(@"Copy failed %@.\n", [e reason]);
        } @finally {
            [fileHandle closeFile];
            [pipeHandle closeFile];
        }
    });
}

- (IBAction)createClicked:(id)sender {
    // [MobileFFmpegConfig setLogDelegate:nil];
    // return;

    [MobileFFmpegConfig setLogDelegate:self];
    NSString *resourceFolder = [[NSBundle mainBundle] resourcePath];
    NSString *image1 = [resourceFolder stringByAppendingPathComponent: @"colosseum.jpg"];
    NSString *image2 = [resourceFolder stringByAppendingPathComponent: @"pyramid.jpg"];
    NSString *image3 = [resourceFolder stringByAppendingPathComponent: @"tajmahal.jpg"];
    NSString *videoFile = [self getVideoPath];

    NSString *pipe1 = [MobileFFmpegConfig registerNewFFmpegPipe];
    NSString *pipe2 = [MobileFFmpegConfig registerNewFFmpegPipe];
    NSString *pipe3 = [MobileFFmpegConfig registerNewFFmpegPipe];
    
    if (player != nil) {
        [player removeAllItems];
        activeItem = nil;
    }

    [[NSFileManager defaultManager] removeItemAtPath:videoFile error:NULL];

    NSLog(@"Testing PIPE with \'mpeg4\' codec\n");

    NSString* ffmpegCommand = [PipeViewController generateCreateVideoWithPipesScript:pipe1:pipe2:pipe3:videoFile];

    [self loadProgressDialog:@"Creating video\n\n"];

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{

        NSLog(@"FFmpeg process started with arguments\n\'%@\'\n", ffmpegCommand);

        // EXECUTE
        int result = [MobileFFmpeg execute: ffmpegCommand];

        NSLog(@"FFmpeg process exited with rc %d\n", result);

        dispatch_async(dispatch_get_main_queue(), ^{
            if (result == RETURN_CODE_SUCCESS) {
                [self dismissProgressDialog];
                NSLog(@"Create completed successfully; playing video.\n");
                [self playVideo];
            } else {
                NSLog(@"Create failed with rc=%d\n", result);

                [self dismissProgressDialogAndAlert:@"Create failed. Please check log for the details."];
            }
        });
    });
    
    // START ASYNC PROCESSES AFTER INITIATING FFMPEG COMMAND
    [PipeViewController startAsyncCopyImageProcess:image1 onPipe:pipe1];
    [PipeViewController startAsyncCopyImageProcess:image2 onPipe:pipe2];
    [PipeViewController startAsyncCopyImageProcess:image3 onPipe:pipe3];
}

- (void)playVideo {
    NSString *videoFile = [self getVideoPath];
    NSURL*videoURL=[NSURL fileURLWithPath:videoFile];

    AVAsset *asset = [AVAsset assetWithURL:videoURL];
    NSArray *assetKeys = @[@"playable", @"hasProtectedContent"];

    AVPlayerItem *newVideo = [AVPlayerItem playerItemWithAsset:asset
                                  automaticallyLoadedAssetKeys:assetKeys];

    NSKeyValueObservingOptions options =
    NSKeyValueObservingOptionOld | NSKeyValueObservingOptionNew;

    activeItem = newVideo;

    [newVideo addObserver:self forKeyPath:@"status" options:options context:nil];

    [player insertItem:newVideo afterItem:nil];
}

- (NSString*)getVideoPath {
    NSString* docFolder = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
    return [docFolder stringByAppendingPathComponent: @"video.mp4"];
}

- (void)loadProgressDialog:(NSString*) dialogMessage {

    // CLEAN STATISTICS
    statistics = nil;
    [MobileFFmpegConfig resetStatistics];

    alertController = [UIAlertController alertControllerWithTitle:nil
                                                                     message:dialogMessage
                                                              preferredStyle:UIAlertControllerStyleAlert];
    indicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhiteLarge];
    indicator.color = [UIColor blackColor];
    indicator.translatesAutoresizingMaskIntoConstraints=NO;
    [alertController.view addSubview:indicator];
    NSDictionary * views = @{@"pending" : alertController.view, @"indicator" : indicator};

    NSArray * constraintsVertical = [NSLayoutConstraint constraintsWithVisualFormat:@"V:[indicator]-(20)-|" options:0 metrics:nil views:views];
    NSArray * constraintsHorizontal = [NSLayoutConstraint constraintsWithVisualFormat:@"H:|[indicator]|" options:0 metrics:nil views:views];
    NSArray * constraints = [constraintsVertical arrayByAddingObjectsFromArray:constraintsHorizontal];
    [alertController.view addConstraints:constraints];
    [indicator startAnimating];
    [self presentViewController:alertController animated:YES completion:nil];
}

- (void)updateProgressDialog {
    if (statistics == nil) {
        return;
    }

    if (alertController != nil) {
        int timeInMilliseconds = [statistics getTime];
        if (timeInMilliseconds > 0) {
            int totalVideoDuration = 9000;

            int percentage = timeInMilliseconds*100/totalVideoDuration;

            [alertController setMessage:[NSString stringWithFormat:@"Creating video  %% %d \n\n", percentage]];
        }
    }
}

- (void)dismissProgressDialog {
    [indicator stopAnimating];
    [self dismissViewControllerAnimated:TRUE completion:nil];
}

- (void)dismissProgressDialogAndAlert: (NSString*)message {
    [indicator stopAnimating];
    [self dismissViewControllerAnimated:TRUE completion:^{
        [Util alert:self withTitle:@"Error" message:message andButtonText:@"OK"];
    }];
}

- (void)setActive {
    [MobileFFmpegConfig setLogDelegate:self];
    [MobileFFmpegConfig setStatisticsDelegate:self];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {

    NSNumber *statusNumber = change[NSKeyValueChangeNewKey];
    NSInteger status = -1;
    if ([statusNumber isKindOfClass:[NSNumber class]]) {
        status = statusNumber.integerValue;
    }

    switch (status) {
        case AVPlayerItemStatusReadyToPlay: {
            [player play];
        } break;
        case AVPlayerItemStatusFailed: {
            if (activeItem != nil && activeItem.error != nil) {

                NSString *message = activeItem.error.localizedFailureReason;
                if (message == nil) {
                    message = activeItem.error.localizedDescription;
                }

                [Util alert:self withTitle:@"Player Error" message:message andButtonText:@"OK"];
            }
        } break;
        default: {
            NSLog(@"Status %ld received from player.\n", status);
        }
    }
}

+ (NSString*)generateCreateVideoWithPipesScript:(NSString *)image1 :(NSString *)image2 :(NSString *)image3 :(NSString *)videoFile {
    return [NSString stringWithFormat:
@"-hide_banner -y -i %@ \
-i %@ \
-i %@ \
-filter_complex \"\
[0:v]loop=loop=-1:size=1:start=0,setpts=PTS-STARTPTS,scale=w=\'if(gte(iw/ih,640/427),min(iw,640),-1)\':h=\'if(gte(iw/ih,640/427),-1,min(ih,427))\',scale=trunc(iw/2)*2:trunc(ih/2)*2,setsar=sar=1/1,split=2[stream1out1][stream1out2];\
[1:v]loop=loop=-1:size=1:start=0,setpts=PTS-STARTPTS,scale=w=\'if(gte(iw/ih,640/427),min(iw,640),-1)\':h=\'if(gte(iw/ih,640/427),-1,min(ih,427))\',scale=trunc(iw/2)*2:trunc(ih/2)*2,setsar=sar=1/1,split=2[stream2out1][stream2out2];\
[2:v]loop=loop=-1:size=1:start=0,setpts=PTS-STARTPTS,scale=w=\'if(gte(iw/ih,640/427),min(iw,640),-1)\':h=\'if(gte(iw/ih,640/427),-1,min(ih,427))\',scale=trunc(iw/2)*2:trunc(ih/2)*2,setsar=sar=1/1,split=2[stream3out1][stream3out2];\
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
-map [video] -vsync 2 -async 1 -c:v mpeg4 -r 30 %@", image1, image2, image3, videoFile];
}

@end
