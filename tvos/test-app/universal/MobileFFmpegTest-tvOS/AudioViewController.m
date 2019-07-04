//
// AudioViewController.m
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

#import <MobileFFmpegConfig.h>
#import <MobileFFmpeg.h>
#import "AudioViewController.h"

@interface AudioViewController ()

@property (strong, nonatomic) IBOutlet UILabel *header;
@property (strong, nonatomic) IBOutlet UITextField *audioCodecText;
@property (strong, nonatomic) IBOutlet UIButton *encodeButton;
@property (strong, nonatomic) IBOutlet UITextView *outputText;

- (void)encodeChromaprint;

@end

@implementation AudioViewController {

    // Video codec data
    NSArray *codecData;
    NSInteger selectedCodec;

    // Loading view
    UIActivityIndicatorView* indicator;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    // STYLE UPDATE
    [Util applyEditTextStyle: self.audioCodecText];
    [Util applyButtonStyle: self.encodeButton];
    [Util applyOutputTextStyle: self.outputText];
    [Util applyHeaderStyle: self.header];

    // BUTTON DISABLED UNTIL AUDIO SAMPLE IS CREATED
    [self.encodeButton setEnabled:false];
    
    [self createAudioSample];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}

- (void)logCallback: (int)level :(NSString*)message {
    dispatch_async(dispatch_get_main_queue(), ^{
        [self appendOutput: message];
    });
}

- (IBAction)encodeClicked:(id)sender {
    // [MobileFFmpegConfig setLogDelegate:nil];
    // [self encodeChromaprint];
    // return;
    
    [MobileFFmpegConfig setLogDelegate:self];
    NSString *audioOutputFile = [self getAudioOutputFilePath];
   
    [[NSFileManager defaultManager] removeItemAtPath:audioOutputFile error:NULL];
    NSString *audioCodec = [[self audioCodecText] text];
    
    NSLog(@"Testing AUDIO encoding with \'%@\' codec\n", audioCodec);
    
    NSString *ffmpegCommand = [self generateAudioEncodeScript];
    
    [self loadProgressDialog:@"Encoding audio\n\n"];

    [self clearOutput];
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        
        NSLog(@"FFmpeg process started with arguments\n\'%@\'\n", ffmpegCommand);
        
        // EXECUTE
        int result = [MobileFFmpeg execute: ffmpegCommand];
        
        NSLog(@"FFmpeg process exited with rc %d\n", result);
        
        dispatch_async(dispatch_get_main_queue(), ^{
            if (result == RETURN_CODE_SUCCESS) {
                NSLog(@"Encode completed successfully.\n");
                [self dismissProgressDialogAndAlert:@"Success" and:@"Encode completed successfully."];
            } else {
                NSLog(@"Encode failed with rc=%d\n", result);
                [self dismissProgressDialogAndAlert:@"Error" and:@"Encode failed. Please check log for the details."];
            }
        });
    });
}

- (void)encodeChromaprint {
    NSString *audioSampleFile = [self getAudioSamplePath];
    
    NSLog(@"Testing AUDIO encoding with 'chromaprint' muxer\n");
    
    NSString* ffmpegCommand = [[NSString alloc] initWithFormat:@"-v quiet -i %@ -f chromaprint -fp_format 2 -", audioSampleFile];
    
    NSLog(@"FFmpeg process started with arguments\n\'%@\'\n", ffmpegCommand);
    
    // EXECUTE
    int result = [MobileFFmpeg execute: ffmpegCommand];
    
    NSLog(@"FFmpeg process exited with rc %d\n", result);
}

- (void)createAudioSample {
    [MobileFFmpegConfig setLogDelegate:nil];
    
    NSLog(@"Creating AUDIO sample before the test.\n");
    
    NSString *audioSampleFile = [self getAudioSamplePath];
    [[NSFileManager defaultManager] removeItemAtPath:audioSampleFile error:NULL];
    
    NSString *ffmpegCommand = [NSString stringWithFormat:@"-y -f lavfi -i sine=frequency=1000:duration=5 -c:a pcm_s16le %@", audioSampleFile];
    
    NSLog(@"Sample file is created with \'%@\'\n", ffmpegCommand);
    
    int result = [MobileFFmpeg execute: ffmpegCommand];
    if (result == RETURN_CODE_SUCCESS) {
        [self.encodeButton setEnabled:true];

        dispatch_async(dispatch_get_main_queue(), ^{
            [self setActive];
        });

        NSLog(@"AUDIO sample created\n");
    } else {
        NSLog(@"Creating AUDIO sample failed with rc=%d\n", result);
        [Util alert:self withTitle:@"Error" message:@"Creating AUDIO sample failed. Please check log for the details." andButtonText:@"OK"];
    }
}

- (NSString*)getAudioOutputFilePath {
    NSString *audioCodec = [[self audioCodecText] text];
    
    NSString *extension;
    if ([audioCodec containsString:@"aac"]) {
        extension = @"m4a";
    } else if ([audioCodec containsString:@"mp2"]) {
        extension = @"mpg";
    } else if ([audioCodec containsString:@"mp3"]) {
        extension = @"mp3";
    } else if ([audioCodec containsString:@"vorbis"]) {
        extension = @"ogg";
    } else if ([audioCodec containsString:@"opus"]) {
        extension = @"opus";
    } else if ([audioCodec containsString:@"amr"]) {
        extension = @"amr";
    } else if ([audioCodec containsString:@"ilbc"]) {
        extension = @"lbc";
    } else if ([audioCodec containsString:@"speex"]) {
        extension = @"spx";
    } else if ([audioCodec containsString:@"wavpack"]) {
        extension = @"wv";
    } else {
        
        // soxr
        extension = @"wav";
    }
    
    NSString* docFolder = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
    return [[docFolder stringByAppendingPathComponent: @"audio."] stringByAppendingString: extension];
}

- (NSString*)getAudioSamplePath {
    NSString* docFolder = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
    return [docFolder stringByAppendingPathComponent: @"audio-sample.wav"];
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

- (void)dismissProgressDialog {
    [indicator stopAnimating];
    [self dismissViewControllerAnimated:TRUE completion:nil];
}

- (void)dismissProgressDialogAndAlert: (NSString*)title and:(NSString*)message {
    [indicator stopAnimating];
    [self dismissViewControllerAnimated:TRUE completion:^{
        [Util alert:self withTitle:title message:message andButtonText:@"OK"];
    }];
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

- (NSString*)generateAudioEncodeScript {
    NSString *audioCodec = [[self audioCodecText] text];
    NSString *audioSampleFile = [self getAudioSamplePath];
    NSString *audioOutputFile = [self getAudioOutputFilePath];

    if ([audioCodec containsString:@"aac"]) {
        return [NSString stringWithFormat:@"-hide_banner -y -i %@ -c:a aac_at -b:a 192k %@", audioSampleFile, audioOutputFile];
    } else if ([audioCodec containsString:@"mp2"]) {
        return [NSString stringWithFormat:@"-hide_banner -y -i %@ -c:a mp2 -b:a 192k %@", audioSampleFile, audioOutputFile];
    } else if ([audioCodec containsString:@"mp3"]) {
        return [NSString stringWithFormat:@"-hide_banner -y -i %@ -c:a libmp3lame -qscale:a 2 %@", audioSampleFile, audioOutputFile];
    } else if ([audioCodec containsString:@"mp3 (libshine)"]) {
        return [NSString stringWithFormat:@"-hide_banner -y -i %@ -c:a libshine -qscale:a 2 %@", audioSampleFile, audioOutputFile];
    } else if ([audioCodec containsString:@"vorbis"]) {
        return [NSString stringWithFormat:@"-hide_banner -y -i %@ -c:a libvorbis -b:a 64k %@", audioSampleFile, audioOutputFile];
    } else if ([audioCodec containsString:@"opus"]) {
        return [NSString stringWithFormat:@"-hide_banner -y -i %@ -c:a libopus -b:a 64k -vbr on -compression_level 10 %@", audioSampleFile, audioOutputFile];
    } else if ([audioCodec containsString:@"amr"]) {
        return [NSString stringWithFormat:@"-hide_banner -y -i %@ -ar 8000 -ab 12.2k -c:a libopencore_amrnb %@", audioSampleFile, audioOutputFile];
    } else if ([audioCodec containsString:@"ilbc"]) {
        return [NSString stringWithFormat:@"-hide_banner -y -i %@ -c:a ilbc -ar 8000 -b:a 15200 %@", audioSampleFile, audioOutputFile];
    } else if ([audioCodec containsString:@"speex"]) {
        return [NSString stringWithFormat:@"-hide_banner -y -i %@ -c:a libspeex -ar 16000 %@", audioSampleFile, audioOutputFile];
    } else if ([audioCodec containsString:@"wavpack"]) {
        return [NSString stringWithFormat:@"-hide_banner -y -i %@ -c:a wavpack -b:a 64k %@", audioSampleFile, audioOutputFile];
    } else {
        
        // soxr
        return [NSString stringWithFormat:@"-hide_banner -y -i %@ -af aresample=resampler=soxr -ar 44100 %@", audioSampleFile, audioOutputFile];
    }
}

@end
