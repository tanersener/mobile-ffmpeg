//
// SecondViewController.m
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

#import "SecondViewController.h"
#import <AVFoundation/AVFoundation.h>
#import <AVKit/AVKit.h>
#import <mobileffmpeg/MobileFFmpeg.h>

NSString * const DEFAULT_VIDEO_CODEC = @"mpeg4";

@interface SecondViewController ()

@property (strong, nonatomic) IBOutlet UILabel *videoPlayerBox;
@property (strong, nonatomic) IBOutlet UIButton *playButton;
@property (strong, nonatomic) IBOutlet UITextField *videoCodecText;

@end

@implementation SecondViewController {
    AVPlayer *player;
    AVPlayerLayer *playerLayer;
    UIActivityIndicatorView* indicator;
}

- (void)viewDidLoad {
    [super viewDidLoad];

    // SET DEFAULT TEXT
    [[self videoCodecText] setText: DEFAULT_VIDEO_CODEC];
    
    // DISABLE PLAY BUTTON
    [[self playButton] setEnabled:FALSE];
    [[self playButton] setUserInteractionEnabled:FALSE];
    
    // CREATING VIDEO PLAYER BOX
    CGRect innerFrame = self.view.bounds;
    innerFrame.size.width = self.view.bounds.size.width - 20;
    innerFrame.size.height = innerFrame.size.width*2/3;
    innerFrame.origin.x = 10;
    innerFrame.origin.y = 290;

    self.videoPlayerBox.frame = innerFrame;
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}

- (IBAction)createClicked:(id)sender {
    NSString *resourceFolder = [[NSBundle mainBundle] resourcePath];
    NSString *image1 = [resourceFolder stringByAppendingPathComponent: @"colosseum.jpg"];
    NSString *image2 = [resourceFolder stringByAppendingPathComponent: @"pyramid.jpg"];
    NSString *image3 = [resourceFolder stringByAppendingPathComponent: @"tajmahal.jpg"];
    NSString *videoFile = [self getVideoPath];

    if (player != nil) {
        [player pause];
        [playerLayer removeFromSuperlayer];
        player = nil;
        playerLayer = nil;
    }

    [[NSFileManager defaultManager] removeItemAtPath:videoFile error:NULL];
    NSString *videoCodec = [[self videoCodecText] text];
    if (videoCodec == nil || [videoCodec length] == 0) {
        videoCodec = DEFAULT_VIDEO_CODEC;
    }

    NSLog(@"Creating slideshow using video codec: %@\n", videoCodec);

    NSString* slideshowCommand = [self generateSlideshowScript:image1:image2:image3:videoFile:videoCodec:[self getCustomOptions]];

    NSLog(@"Creating slideshow: %@\n", slideshowCommand);
    [self loadProgressDialog:@"Creating video slideshow\n\n"];

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{

        // EXECUTE
        int result = [MobileFFmpeg execute: slideshowCommand];

        dispatch_async(dispatch_get_main_queue(), ^{
            [self dismissProgressDialog];
            
            [[self playButton] setEnabled:TRUE];
            [[self playButton] setUserInteractionEnabled:TRUE];
        });
        
        if (result != 0) {
            NSLog(@"Create failed with rc=%d\n", result);
        }
    });
}

- (IBAction)playClicked:(id)sender {
    NSString *videoFile = [self getVideoPath];
    NSURL*videoURL=[NSURL fileURLWithPath:videoFile];

    player = [AVPlayer playerWithURL:videoURL];
    playerLayer = [AVPlayerLayer playerLayerWithPlayer:player];
    
    CGRect innerFrame = self.view.bounds;
    innerFrame.size.width = self.view.bounds.size.width - 20;
    innerFrame.origin.x = 10;
    innerFrame.origin.y = 100;
    
    playerLayer.frame = innerFrame;

    [self.view.layer addSublayer:playerLayer];
    [player play];
}

- (NSString *) generateSlideshowScript:(NSString *)image1 :(NSString *)image2 :(NSString *)image3 :(NSString *)videoFile :(NSString *)videoCodec :(NSString *)customOptions {
    return [NSString stringWithFormat:
@"-loop 1 -i %@ \
-loop 1 -i %@ \
-loop 1 -i %@ \
-filter_complex \
[0:v]setpts=PTS-STARTPTS,scale=w=\'if(gte(iw/ih,640/427),min(iw,640),-1)\':h=\'if(gte(iw/ih,640/427),-1,min(ih,427))\',scale=trunc(iw/2)*2:trunc(ih/2)*2,setsar=sar=1/1,split=2[stream1out1][stream1out2];\
[1:v]setpts=PTS-STARTPTS,scale=w=\'if(gte(iw/ih,640/427),min(iw,640),-1)\':h=\'if(gte(iw/ih,640/427),-1,min(ih,427))\',scale=trunc(iw/2)*2:trunc(ih/2)*2,setsar=sar=1/1,split=2[stream2out1][stream2out2];\
[2:v]setpts=PTS-STARTPTS,scale=w=\'if(gte(iw/ih,640/427),min(iw,640),-1)\':h=\'if(gte(iw/ih,640/427),-1,min(ih,427))\',scale=trunc(iw/2)*2:trunc(ih/2)*2,setsar=sar=1/1,split=2[stream3out1][stream3out2];\
[stream1out1]pad=width=640:height=427:x=(640-iw)/2:y=(427-ih)/2:color=#00000000,trim=duration=3,select=lte(n\\,90)[stream1overlaid];\
[stream1out2]pad=width=640:height=427:x=(640-iw)/2:y=(427-ih)/2:color=#00000000,trim=duration=1,select=lte(n\\,30)[stream1ending];\
[stream2out1]pad=width=640:height=427:x=(640-iw)/2:y=(427-ih)/2:color=#00000000,trim=duration=2,select=lte(n\\,60)[stream2overlaid];\
[stream2out2]pad=width=640:height=427:x=(640-iw)/2:y=(427-ih)/2:color=#00000000,trim=duration=1,select=lte(n\\,30),split=2[stream2starting][stream2ending];\
[stream3out1]pad=width=640:height=427:x=(640-iw)/2:y=(427-ih)/2:color=#00000000,trim=duration=2,select=lte(n\\,60)[stream3overlaid];\
[stream3out2]pad=width=640:height=427:x=(640-iw)/2:y=(427-ih)/2:color=#00000000,trim=duration=1,select=lte(n\\,30)[stream3starting];\
[stream2starting][stream1ending]blend=all_expr=\'if(gte(X,(W/2)*T/1)*lte(X,W-(W/2)*T/1),B,A)\':shortest=1[stream2blended];\
[stream3starting][stream2ending]blend=all_expr=\'if(gte(X,(W/2)*T/1)*lte(X,W-(W/2)*T/1),B,A)\':shortest=1[stream3blended];\
[stream1overlaid][stream2blended][stream2overlaid][stream3blended][stream3overlaid]concat=n=5:v=1:a=0,format=yuv420p[video] \
-map [video] -vsync 2 -async 1 %@-c:v %@ -r 30 %@", image1, image2, image3, customOptions, videoCodec, videoFile];
}

- (void)loadProgressDialog:(NSString*) dialogMessage {
    UIAlertController *pending = [UIAlertController alertControllerWithTitle:nil
                                                                     message:dialogMessage
                                                              preferredStyle:UIAlertControllerStyleAlert];
    indicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhiteLarge];
    indicator.color = [UIColor blackColor];
    indicator.translatesAutoresizingMaskIntoConstraints=NO;
    [pending.view addSubview:indicator];
    NSDictionary * views = @{@"pending" : pending.view, @"indicator" : indicator};
    
    NSArray * constraintsVertical = [NSLayoutConstraint constraintsWithVisualFormat:@"V:[indicator]-(20)-|" options:0 metrics:nil views:views];
    NSArray * constraintsHorizontal = [NSLayoutConstraint constraintsWithVisualFormat:@"H:|[indicator]|" options:0 metrics:nil views:views];
    NSArray * constraints = [constraintsVertical arrayByAddingObjectsFromArray:constraintsHorizontal];
    [pending.view addConstraints:constraints];
    [indicator startAnimating];
    [self presentViewController:pending animated:YES completion:nil];
}

- (NSString *) getVideoPath {
    NSString *videoCodec = [[self videoCodecText] text];
    if (videoCodec == nil || [videoCodec length] == 0) {
        videoCodec = DEFAULT_VIDEO_CODEC;
    }
    
    NSString *extension;
    if ([videoCodec isEqualToString:@"libaom-av1"]) {
        extension = @"mkv";
    } else {
        extension = @"mp4";
    }
    
    NSString* docFolder = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
    return [[docFolder stringByAppendingPathComponent: @"slideshow."] stringByAppendingString: extension];
}

- (NSString *) getCustomOptions {
    NSString *videoCodec = [[self videoCodecText] text];
    if (videoCodec == nil || [videoCodec length] == 0) {
        videoCodec = DEFAULT_VIDEO_CODEC;
    }
    
    if ([videoCodec isEqualToString:@"libaom-av1"]) {
        return @"-strict experimental ";
    } else {
        return @"";
    }
}

- (void)dismissProgressDialog {
    [indicator stopAnimating];
    [self dismissViewControllerAnimated:TRUE completion:nil];
}

@end
