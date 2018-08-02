//
// CommandViewController.m
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

#import "CommandViewController.h"
#import "RCEasyTipView.h"
#import <mobileffmpeg/Log.h>
#import <mobileffmpeg/MobileFFmpeg.h>

@interface CommandViewController ()

@property (strong, nonatomic) IBOutlet UILabel *header;
@property (strong, nonatomic) IBOutlet UITextField *commandText;
@property (strong, nonatomic) IBOutlet UIButton *runButton;
@property (strong, nonatomic) IBOutlet UIButton *runAsyncButton;
@property (strong, nonatomic) IBOutlet UITextView *outputText;

@end

@implementation CommandViewController {

    // Tooltip view reference
    RCEasyTipView *tooltip;    
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    [Util applyEditTextStyle: self.commandText];
    
    [Util applyButtonStyle: self.runButton];
    
    [Util applyButtonStyle: self.runAsyncButton];
    
    [Util applyOutputTextStyle: self.outputText];
    
    [Util applyHeaderStyle: self.header];

    RCEasyTipPreferences *preferences = [[RCEasyTipPreferences alloc] initWithDefaultPreferences];
    [Util applyTooltipStyle: preferences];
    preferences.drawing.arrowPostion = Top;
    preferences.animating.showDuration = 1.0;
    preferences.animating.dismissDuration = COMMAND_TEST_TOOLTIP_DURATION;
    preferences.animating.dismissTransform = CGAffineTransformMakeTranslation(0, -15);
    preferences.animating.showInitialTransform = CGAffineTransformMakeTranslation(0, -15);

    tooltip = [[RCEasyTipView alloc] initWithPreferences:preferences];
    tooltip.text = COMMAND_TEST_TOOLTIP_TEXT;
    
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

- (IBAction)runAction:(id)sender {
    [self hideTooltip];
    
    [self clearOutput];
    
    NSString *ffmpegCommand = [[self commandText] text];

    NSLog(@"Testing COMMAND synchronously.\n");
    
    NSLog(@"FFmpeg process started with arguments\n\'%@\'\n", ffmpegCommand);
    
    // EXECUTE
    int result = [MobileFFmpeg execute: ffmpegCommand];
    
    NSLog(@"FFmpeg process exited with rc %d\n", result);
}

- (IBAction)runAsyncAction:(id)sender {
    [self hideTooltip];
    
    [self clearOutput];

    NSString *ffmpegCommand = [[self commandText] text];
    
    NSLog(@"Testing COMMAND asynchronously.\n");
    
    dispatch_async(dispatch_get_main_queue(), ^{
        
        NSLog(@"FFmpeg process started with arguments\n\'%@\'\n", ffmpegCommand);
        
        // EXECUTE
        int result = [MobileFFmpeg execute: ffmpegCommand];
        
        NSLog(@"FFmpeg process exited with rc %d\n", result);
    });
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
    [tooltip showAnimated:YES forView:self.commandText withinSuperView:self.view];
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
