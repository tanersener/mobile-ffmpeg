//
// AudioViewController.m
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

#import "AudioViewController.h"
#import "RCEasyTipView.h"
#import <mobileffmpeg/Log.h>
#import <mobileffmpeg/MobileFFmpeg.h>

@interface AudioViewController ()

@property (strong, nonatomic) IBOutlet UILabel *header;
@property (strong, nonatomic) IBOutlet UIPickerView *audioCodecPicker;
@property (strong, nonatomic) IBOutlet UIButton *encodeButton;
@property (strong, nonatomic) IBOutlet UITextView *outputText;

@end

@implementation AudioViewController {

    // Video codec data
    NSArray *codecData;
    NSInteger selectedCodec;

    // Loading view
    UIActivityIndicatorView* indicator;

    // Tooltip view reference
    RCEasyTipView *tooltip;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    // AUDIO CODEC PICKER INIT
    codecData = @[@"mp3 (liblame)", @"mp3 (libshine)", @"vorbis", @"opus", @"amr", @"ilbc", @"soxr", @"speex", @"wavpack"];
    selectedCodec = 0;
    
    self.audioCodecPicker.dataSource = self;
    self.audioCodecPicker.delegate = self;

    // STYLE UPDATE
    [Util applyPickerViewStyle: self.audioCodecPicker];
    [Util applyButtonStyle: self.encodeButton];
    [Util applyOutputTextStyle: self.outputText];
    [Util applyHeaderStyle: self.header];
    
    // TOOLTIP INIT
    RCEasyTipPreferences *preferences = [[RCEasyTipPreferences alloc] initWithDefaultPreferences];
    [Util applyTooltipStyle: preferences];
    preferences.drawing.arrowPostion = Top;
    preferences.animating.showDuration = 1.0;
    preferences.animating.dismissDuration = AUDIO_TEST_TOOLTIP_DURATION;
    preferences.animating.dismissTransform = CGAffineTransformMakeTranslation(0, -15);
    preferences.animating.showInitialTransform = CGAffineTransformMakeTranslation(0, -15);
    
    tooltip = [[RCEasyTipView alloc] initWithPreferences:preferences];
    tooltip.text = AUDIO_TEST_TOOLTIP_TEXT;

    // BUTTON DISABLED UNTIL AUDIO SAMPLE IS CREATED
    [self.encodeButton setEnabled:false];
    
    [self createAudioSample];
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
        [self appendOutput: message];
    });
}

- (IBAction)encodeClicked:(id)sender {
    NSString *audioOutputFile = [self getAudioOutputFilePath];
   
    [[NSFileManager defaultManager] removeItemAtPath:audioOutputFile error:NULL];
    NSString *audioCodec = codecData[selectedCodec];
    
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
            if (result == 0) {
                NSLog(@"Encode completed successfully.\n");
                [self dismissProgressDialogAndAlert:@"Success" and:@"Encode completed successfully."];
            } else {
                NSLog(@"Encode failed with rc=%d\n", result);
                [self dismissProgressDialogAndAlert:@"Error" and:@"Encode failed. Please check log for the details."];
            }
        });
    });

}

- (void)createAudioSample {
    [Log setLogDelegate:nil];
    
    NSLog(@"Creating AUDIO sample before the test.\n");
    
    NSString *audioSampleFile = [self getAudioSamplePath];
    [[NSFileManager defaultManager] removeItemAtPath:audioSampleFile error:NULL];
    
    NSString *ffmpegCommand = [NSString stringWithFormat:@"-y -f lavfi -i sine=frequency=1000:duration=5 -c:a pcm_s16le %@", audioSampleFile];
    
    NSLog(@"Sample file is %@\n", ffmpegCommand);
    
    int result = [MobileFFmpeg execute: ffmpegCommand];
    if (result == 0) {
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
    NSString *audioCodec = codecData[selectedCodec];
    
    NSString *extension;
    if ([audioCodec isEqualToString:@"mp3 (liblame)"] || [audioCodec isEqualToString:@"mp3 (libshine)"]) {
        extension = @"mp3";
    } else if ([audioCodec isEqualToString:@"vorbis"]) {
        extension = @"ogg";
    } else if ([audioCodec isEqualToString:@"opus"]) {
        extension = @"opus";
    } else if ([audioCodec isEqualToString:@"amr"]) {
        extension = @"amr";
    } else if ([audioCodec isEqualToString:@"ilbc"]) {
        extension = @"lbc";
    } else if ([audioCodec isEqualToString:@"speex"]) {
        extension = @"spx";
    } else if ([audioCodec isEqualToString:@"wavpack"]) {
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
    [Log setLogDelegate:self];
    [self hideTooltip];
    [self showTooltip];
}

- (void)hideTooltip {
    [tooltip dismissWithCompletion:nil];
}

- (void)showTooltip {
    [tooltip showAnimated:YES forView:self.encodeButton withinSuperView:self.view];
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
    NSString *audioCodec = codecData[selectedCodec];
    NSString *audioSampleFile = [self getAudioSamplePath];
    NSString *audioOutputFile = [self getAudioOutputFilePath];

    if ([audioCodec isEqualToString:@"mp3 (liblame)"]) {
        return [NSString stringWithFormat:@"-y -i %@ -c:a libmp3lame -qscale:a 2 %@", audioSampleFile, audioOutputFile];
    } else if ([audioCodec isEqualToString:@"mp3 (libshine)"]) {
        return [NSString stringWithFormat:@"-y -i %@ -c:a libshine -qscale:a 2 %@", audioSampleFile, audioOutputFile];
    } else if ([audioCodec isEqualToString:@"vorbis"]) {
        return [NSString stringWithFormat:@"-y -i %@ -c:a libvorbis -b:a 64k %@", audioSampleFile, audioOutputFile];
    } else if ([audioCodec isEqualToString:@"opus"]) {
        return [NSString stringWithFormat:@"-y -i %@ -c:a libopus -b:a 64k -vbr on -compression_level 10 %@", audioSampleFile, audioOutputFile];
    } else if ([audioCodec isEqualToString:@"amr"]) {
        return [NSString stringWithFormat:@"-y -i %@ -ar 8000 -ab 12.2k %@", audioSampleFile, audioOutputFile];
    } else if ([audioCodec isEqualToString:@"ilbc"]) {
        return [NSString stringWithFormat:@"-y -i %@ -c:a ilbc -ar 8000 -b:a 15200 %@", audioSampleFile, audioOutputFile];
    } else if ([audioCodec isEqualToString:@"speex"]) {
        return [NSString stringWithFormat:@"-y -i %@ -c:a libspeex -ar 16000 %@", audioSampleFile, audioOutputFile];
    } else if ([audioCodec isEqualToString:@"wavpack"]) {
        return [NSString stringWithFormat:@"-y -i %@ -c:a wavpack -b:a 64k %@", audioSampleFile, audioOutputFile];
    } else {
        
        // soxr
        return [NSString stringWithFormat:@"-y -i %@ -af aresample=resampler=soxr -ar 44100 %@", audioSampleFile, audioOutputFile];
    }
}

@end
