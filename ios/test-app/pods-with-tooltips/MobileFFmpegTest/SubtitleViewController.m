//
// SubtitleViewController.m
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
#import "SubtitleViewController.h"
#import "VideoViewController.h"

typedef enum {
    IdleState = 1,
    CreatingState = 2,
    BurningState = 3
} UITestState;

@interface SubtitleViewController ()

@property (strong, nonatomic) IBOutlet UILabel *header;
@property (strong, nonatomic) IBOutlet UIButton *burnSubtitlesButton;
@property (strong, nonatomic) IBOutlet UILabel *videoPlayerFrame;

@end

@implementation SubtitleViewController  {
    
    // Video player references
    AVQueuePlayer *player;
    AVPlayerLayer *playerLayer;
    
    // Loading view
    UIAlertController *alertController;
    UIActivityIndicatorView* indicator;
    
    // Tooltip view reference
    RCEasyTipView *tooltip;

    Statistics *statistics;
    
    UITestState state;
}

- (void)viewDidLoad {
    [super viewDidLoad];

    // STYLE UPDATE
    [Util applyButtonStyle: self.burnSubtitlesButton];
    [Util applyVideoPlayerFrameStyle: self.videoPlayerFrame];    
    [Util applyHeaderStyle: self.header];

    // TOOLTIP INIT
    RCEasyTipPreferences *preferences = [[RCEasyTipPreferences alloc] initWithDefaultPreferences];
    [Util applyTooltipStyle: preferences];
    preferences.drawing.arrowPostion = Top;
    preferences.animating.showDuration = 1.0;
    preferences.animating.dismissDuration = SUBTITLE_TEST_TOOLTIP_DURATION;
    preferences.animating.dismissTransform = CGAffineTransformMakeTranslation(0, -15);
    preferences.animating.showInitialTransform = CGAffineTransformMakeTranslation(0, -15);

    tooltip = [[RCEasyTipView alloc] initWithPreferences:preferences];
    tooltip.text = SUBTITLE_TEST_TOOLTIP_TEXT;

    // VIDEO PLAYER INIT
    player = [[AVQueuePlayer alloc] init];
    playerLayer = [AVPlayerLayer playerLayerWithPlayer:player];
    
    CGRect rectangularFrame = self.view.layer.bounds;
    rectangularFrame.size.width = self.view.layer.bounds.size.width - 40;
    rectangularFrame.origin.x = 20;
    rectangularFrame.origin.y = self.burnSubtitlesButton.layer.bounds.origin.y + 80;
    
    playerLayer.frame = rectangularFrame;
    [self.view.layer addSublayer:playerLayer];
    
    alertController = nil;
    statistics = nil;

    state = IdleState;

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

- (IBAction)burnSubtitlesClicked:(id)sender {
    NSString *resourceFolder = [[NSBundle mainBundle] resourcePath];
    NSString *image1 = [resourceFolder stringByAppendingPathComponent: @"colosseum.jpg"];
    NSString *image2 = [resourceFolder stringByAppendingPathComponent: @"pyramid.jpg"];
    NSString *image3 = [resourceFolder stringByAppendingPathComponent: @"tajmahal.jpg"];
    NSString *subtitle = [resourceFolder stringByAppendingPathComponent: @"subtitle.srt"];
    NSString *videoFile = [self getVideoPath];
    NSString *videoWithSubtitlesFile = [self getVideoWithSubtitlesPath];
    
    if (player != nil) {
        [player removeAllItems];
    }

    [[NSFileManager defaultManager] removeItemAtPath:videoFile error:NULL];
    [[NSFileManager defaultManager] removeItemAtPath:videoWithSubtitlesFile error:NULL];

    NSLog(@"Testing SUBTITLE burning\n");

    [self loadProgressDialog:@"Creating video\n\n"];

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
    
        NSString* ffmpegCommand = [VideoViewController generateVideoEncodeScript:image1:image2:image3:videoFile:@"mpeg4":@""];
        
        NSLog(@"FFmpeg process started with arguments\n\'%@\'\n", ffmpegCommand);
        
        self->state = CreatingState;
        
        // EXECUTE
        int result = [MobileFFmpeg execute: ffmpegCommand];
        
        NSLog(@"FFmpeg process exited with rc %d\n", result);
        
        dispatch_async(dispatch_get_main_queue(), ^{
            self->state = IdleState;

            if (result == RETURN_CODE_SUCCESS) {
                [self dismissProgressDialog];
                
                NSLog(@"Create completed successfully; burning subtitles.\n");

                NSString *burnSubtitlesCommand = [NSString stringWithFormat:@"-hide_banner -y -i %@ -vf subtitles=%@:force_style='FontName=MyFontName' %@", videoFile, subtitle, videoWithSubtitlesFile];
                
                [self loadProgressDialog:@"Burning subtitles\n\n"];
                
                dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                    
                    NSLog(@"FFmpeg process started with arguments\n\'%@\'\n", burnSubtitlesCommand);
                    
                    self->state = BurningState;
                    
                    // EXECUTE
                    int result = [MobileFFmpeg execute: burnSubtitlesCommand];
                    
                    NSLog(@"FFmpeg process exited with rc %d\n", result);
                    
                    dispatch_async(dispatch_get_main_queue(), ^{
                        self->state = IdleState;

                        if (result == RETURN_CODE_SUCCESS) {
                            [self dismissProgressDialog];
                            
                            NSLog(@"Burn subtitles completed successfully; playing video.\n");
                            [self playVideo];
                        } else if (result == RETURN_CODE_CANCEL) {
                            NSLog(@"Burn subtitles operation cancelled\n");
                            
                            [self->indicator stopAnimating];
                            [Util alert:self withTitle:@"Error" message:@"Burn subtitles operation cancelled." andButtonText:@"OK"];
                        } else {
                            NSLog(@"Burn subtitles failed with rc=%d\n", result);

                            dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, 1.3 * NSEC_PER_SEC);
                            dispatch_after(popTime, dispatch_get_main_queue(), ^(void){
                                [self dismissProgressDialogAndAlert:@"Burn subtitles failed. Please check log for the details."];
                            });
                        }
                    });
                });
            } else if (result == RETURN_CODE_CANCEL) {
                NSLog(@"Create operation cancelled\n");

                [self->indicator stopAnimating];
                [Util alert:self withTitle:@"Error" message:@"Create operation cancelled." andButtonText:@"OK"];
            } else {
                NSLog(@"Create failed with rc=%d\n", result);
                
                [self dismissProgressDialogAndAlert:@"Create video failed. Please check log for the details."];
            }
        });
    });
}

- (void)playVideo {
    NSString *videoWithSubtitlesFile = [self getVideoWithSubtitlesPath];
    NSURL*videoWithSubtitlesURL=[NSURL fileURLWithPath:videoWithSubtitlesFile];
    
    AVAsset *asset = [AVAsset assetWithURL:videoWithSubtitlesURL];
    NSArray *assetKeys = @[@"playable", @"hasProtectedContent"];
    
    AVPlayerItem *newVideo = [AVPlayerItem playerItemWithAsset:asset
                                  automaticallyLoadedAssetKeys:assetKeys];
    
    [player insertItem:newVideo afterItem:nil];
    [player play];
}

- (NSString*)getVideoPath {
    NSString* docFolder = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
    return [docFolder stringByAppendingPathComponent: @"video.mp4"];
}

- (NSString*)getVideoWithSubtitlesPath {
    NSString* docFolder = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
    return [docFolder stringByAppendingPathComponent: @"video-with-subtitles.mp4"];
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
    
    UIAlertAction* cancelAction = [UIAlertAction actionWithTitle:@"CANCEL" style:UIAlertActionStyleDefault
                                                          handler:^(UIAlertAction * action) {
                                                              [MobileFFmpeg cancel];
                                                          }];
    [alertController addAction:cancelAction];

    NSArray * constraintsVertical = [NSLayoutConstraint constraintsWithVisualFormat:@"V:[indicator]-(56)-|" options:0 metrics:nil views:views];
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
            
            if (state == CreatingState) {
                [alertController setMessage:[NSString stringWithFormat:@"Creating video  %% %d \n\n", percentage]];
            } else if (state == BurningState) {
                [alertController setMessage:[NSString stringWithFormat:@"Burning subtitles  %% %d \n\n", percentage]];
            }            
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
    [tooltip showAnimated:YES forView:self.burnSubtitlesButton withinSuperView:self.view];
}

@end
