//
// VideoViewController.m
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

#import <AVFoundation/AVFoundation.h>
#import <AVKit/AVKit.h>
#import <mobileffmpeg/MobileFFmpegConfig.h>
#import <mobileffmpeg/MobileFFmpeg.h>
#import "RCEasyTipView.h"
#import "VideoViewController.h"

@interface VideoViewController ()

@property (strong, nonatomic) IBOutlet UILabel *header;
@property (strong, nonatomic) IBOutlet UIPickerView *videoCodecPicker;
@property (strong, nonatomic) IBOutlet UIButton *encodeButton;
@property (strong, nonatomic) IBOutlet UILabel *videoPlayerFrame;

- (void)encodeWebp;

@end

@implementation VideoViewController {

    // Video codec data
    NSArray *codecData;
    NSInteger selectedCodec;
    
    // Video player references
    AVQueuePlayer *player;
    AVPlayerLayer *playerLayer;
    AVPlayerItem *activeItem;
    
    // Loading view
    UIAlertController *alertController;
    UIActivityIndicatorView* indicator;

    // Tooltip view reference
    RCEasyTipView *tooltip;
    
    Statistics *statistics;
}

- (void)viewDidLoad {
    [super viewDidLoad];

    // VIDEO CODEC PICKER INIT
    codecData = @[@"mpeg4", @"h264 (x264)", @"h264 (videotoolbox)", @"x265", @"xvid", @"vp8", @"vp9", @"aom", @"kvazaar", @"theora", @"hap"];
    selectedCodec = 0;
    
    self.videoCodecPicker.dataSource = self;
    self.videoCodecPicker.delegate = self;

    // STYLE UPDATE
    [Util applyButtonStyle: self.encodeButton];
    [Util applyPickerViewStyle: self.videoCodecPicker];
    [Util applyVideoPlayerFrameStyle: self.videoPlayerFrame];
    [Util applyHeaderStyle: self.header];
    
    // TOOLTIP INIT
    RCEasyTipPreferences *preferences = [[RCEasyTipPreferences alloc] initWithDefaultPreferences];
    [Util applyTooltipStyle: preferences];
    preferences.drawing.arrowPostion = Top;
    preferences.animating.showDuration = 1.0;
    preferences.animating.dismissDuration = VIDEO_TEST_TOOLTIP_DURATION;
    preferences.animating.dismissTransform = CGAffineTransformMakeTranslation(0, -15);
    preferences.animating.showInitialTransform = CGAffineTransformMakeTranslation(0, -15);
    
    tooltip = [[RCEasyTipView alloc] initWithPreferences:preferences];
    tooltip.text = VIDEO_TEST_TOOLTIP_TEXT;
    
    // VIDEO PLAYER INIT
    player = [[AVQueuePlayer alloc] init];
    playerLayer = [AVPlayerLayer playerLayerWithPlayer:player];
    activeItem = nil;
    
    CGRect rectangularFrame = self.view.layer.bounds;
    rectangularFrame.size.width = self.view.layer.bounds.size.width - 40;
    rectangularFrame.origin.x = 20;
    rectangularFrame.origin.y = self.encodeButton.layer.bounds.origin.y + 120;
    
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

/**
 * The number of columns of data
 */
- (NSInteger)numberOfComponentsInPickerView:(UIPickerView *)pickerView {
    return 1;
}

/**
 * The number of rows of data
 */
- (NSInteger)pickerView:(UIPickerView *)pickerView numberOfRowsInComponent:(NSInteger)component {
    return codecData.count;
}

/**
 * The data to return for the row and component (column) that's being passed in
 */
- (NSString*)pickerView:(UIPickerView *)pickerView titleForRow:(NSInteger)row forComponent:(NSInteger)component {
    return codecData[row];
}

- (void)pickerView:(UIPickerView *)pickerView didSelectRow:(NSInteger)row inComponent:(NSInteger)component {
    selectedCodec = row;
}

- (void)logCallback: (int)level :(NSString*)message {
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

- (IBAction)encodeClicked:(id)sender {
    // [MobileFFmpegConfig setLogDelegate:nil];
    // [self encodeWebp];
    // return;

    [MobileFFmpegConfig setLogDelegate:self];
    NSString *resourceFolder = [[NSBundle mainBundle] resourcePath];
    NSString *image1 = [resourceFolder stringByAppendingPathComponent: @"colosseum.jpg"];
    NSString *image2 = [resourceFolder stringByAppendingPathComponent: @"pyramid.jpg"];
    NSString *image3 = [resourceFolder stringByAppendingPathComponent: @"tajmahal.jpg"];
    NSString *videoFile = [self getVideoPath];

    if (player != nil) {
        [player removeAllItems];
        activeItem = nil;
    }

    [[NSFileManager defaultManager] removeItemAtPath:videoFile error:NULL];
    NSString *videoCodec = codecData[selectedCodec];

    NSLog(@"Testing VIDEO encoding with \'%@\' codec\n", videoCodec);

    NSString* ffmpegCommand = [VideoViewController generateVideoEncodeScript:image1:image2:image3:videoFile:[self getSelectedVideoCodec]:[self getCustomOptions]];

    [self loadProgressDialog:@"Encoding video\n\n"];

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        
        NSLog(@"FFmpeg process started with arguments\n\'%@\'\n", ffmpegCommand);
        
        // EXECUTE
        int result = [MobileFFmpeg execute: ffmpegCommand];
        
        NSLog(@"FFmpeg process exited with rc %d\n", result);

        dispatch_async(dispatch_get_main_queue(), ^{
            if (result == RETURN_CODE_SUCCESS) {
                [self dismissProgressDialog];
                NSLog(@"Encode completed successfully; playing video.\n");
                [self playVideo];
            } else {
                NSLog(@"Encode failed with rc=%d\n", result);
                
                [self dismissProgressDialogAndAlert:@"Encode failed. Please check log for the details."];
            }
        });
    });
}

- (void)encodeWebp {
    NSString *resourceFolder = [[NSBundle mainBundle] resourcePath];
    NSString *image = [resourceFolder stringByAppendingPathComponent: @"colosseum.jpg"];
    
    NSString *docFolder = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
    NSString *output = [[docFolder stringByAppendingPathComponent: @"video."] stringByAppendingString: @"webp"];

    NSLog(@"Testing VIDEO encoding with \'webp\' codec\n");
    
    NSString* ffmpegCommand = [[NSString alloc] initWithFormat:@"-hide_banner -i %@ %@", image, output];

    NSLog(@"FFmpeg process started with arguments\n\'%@\'\n", ffmpegCommand);
    
    // EXECUTE
    int result = [MobileFFmpeg execute: ffmpegCommand];
    
    NSLog(@"FFmpeg process exited with rc %d\n", result);
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

- (NSString*)getSelectedVideoCodec {
    NSString *videoCodec = codecData[selectedCodec];
    
    // VIDEO CODEC PICKER HAS BASIC NAMES, FFMPEG NEEDS LONGER AND EXACT CODEC NAMES.
    // APPLYING NECESSARY TRANSFORMATION HERE
    if ([videoCodec isEqualToString:@"h264 (x264)"]) {
        videoCodec = @"libx264";
    } else if ([videoCodec isEqualToString:@"h264 (videotoolbox)"]) {
        videoCodec = @"h264_videotoolbox";
    } else if ([videoCodec isEqualToString:@"x265"]) {
        videoCodec = @"libx265";
    } else if ([videoCodec isEqualToString:@"xvid"]) {
        videoCodec = @"libxvid";
    } else if ([videoCodec isEqualToString:@"vp8"]) {
        videoCodec = @"libvpx";
    } else if ([videoCodec isEqualToString:@"vp9"]) {
        videoCodec = @"libvpx-vp9";
    } else if ([videoCodec isEqualToString:@"aom"]) {
        videoCodec = @"libaom-av1";
    } else if ([videoCodec isEqualToString:@"kvazaar"]) {
        videoCodec = @"libkvazaar";
    } else if ([videoCodec isEqualToString:@"theora"]) {
        videoCodec = @"libtheora";
    }
    
    return videoCodec;
}

- (NSString*)getVideoPath {
    NSString *videoCodec = codecData[selectedCodec];
    
    NSString *extension;
    if ([videoCodec isEqualToString:@"vp8"] || [videoCodec isEqualToString:@"vp9"]) {
        extension = @"webm";
    } else if ([videoCodec isEqualToString:@"aom"]) {
        extension = @"mkv";
    } else if ([videoCodec isEqualToString:@"theora"]) {
        extension = @"ogv";
    } else if ([videoCodec isEqualToString:@"hap"]) {
        extension = @"mov";
    } else {
        
        // mpeg4, x264, x265, xvid, kvazaar
        extension = @"mp4";
    }
    
    NSString* docFolder = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
    return [[docFolder stringByAppendingPathComponent: @"video."] stringByAppendingString: extension];
}

- (NSString*)getCustomOptions {
    NSString *videoCodec = codecData[selectedCodec];

    if ([videoCodec isEqualToString:@"x265"]) {
        return @"-crf 28 -preset fast ";
    } else if ([videoCodec isEqualToString:@"vp8"]) {
        return @"-b:v 1M -crf 10 ";
    } else if ([videoCodec isEqualToString:@"vp9"]) {
        return @"-b:v 2M ";
    } else if ([videoCodec isEqualToString:@"aom"]) {
        return @"-crf 30 -strict experimental ";
    } else if ([videoCodec isEqualToString:@"theora"]) {
        return @"-qscale:v 7 ";
    } else if ([videoCodec isEqualToString:@"hap"]) {
        return @"-format hap_q ";
    } else {
        return @"";
    }
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
            
            [alertController setMessage:[NSString stringWithFormat:@"Encoding video  %% %d \n\n", percentage]];
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
    [self hideTooltip];
    [self showTooltip];
}

- (void)hideTooltip {
    [tooltip dismissWithCompletion:nil];
}

- (void)showTooltip {
    [tooltip showAnimated:YES forView:self.encodeButton withinSuperView:self.view];
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

+ (NSString*)generateVideoEncodeScript:(NSString *)image1 :(NSString *)image2 :(NSString *)image3 :(NSString *)videoFile :(NSString *)videoCodec :(NSString *)customOptions {
    return [NSString stringWithFormat:
@"-hide_banner -y -loop 1 -i \"%@\" \
-loop 1 -i '%@' \
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
