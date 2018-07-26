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

#import "HttpsViewController.h"
#import "RCEasyTipView.h"
#import <mobileffmpeg/MobileFFmpeg.h>
#import <mobileffmpeg/Log.h>

@interface HttpsViewController ()

@property (strong, nonatomic) IBOutlet UILabel *header;
@property (strong, nonatomic) IBOutlet UITextField *urlText;
@property (strong, nonatomic) IBOutlet UIButton *getInfoButton;
@property (strong, nonatomic) IBOutlet UITextView *outputText;

@end

@implementation HttpsViewController

/** Tooltip view reference */
RCEasyTipView *tooltip;

- (void)viewDidLoad {
    [super viewDidLoad];
    
    [Util applyEditTextStyle: self.urlText];
    
    [Util applyButtonStyle: self.getInfoButton];

    [Util applyHeaderStyle: self.header];
    
    [Util applyOutputTextStyle: self.outputText];
    
    RCEasyTipPreferences *preferences = [[RCEasyTipPreferences alloc] initWithDefaultPreferences];
    [Util applyTooltipStyle: preferences];
    preferences.drawing.arrowPostion = Top;
    preferences.animating.showDuration = 1.0;
    preferences.animating.dismissDuration = HTTPS_TEST_TOOLTIP_DURATION;
    preferences.animating.dismissTransform = CGAffineTransformMakeTranslation(0, -15);
    preferences.animating.showInitialTransform = CGAffineTransformMakeTranslation(0, -15);
    
    tooltip = [[RCEasyTipView alloc] initWithPreferences:preferences];
    tooltip.text = HTTPS_TEST_TOOLTIP_TEXT;

    [Log setLogDelegate:self];
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
    [Log setLevel:AV_LOG_WARNING];

    [self hideTooltip];
    
    [self clearOutput];
    
    NSString *testUrl = [self.urlText text];
    if ([testUrl length] > 0) {
        [self appendOutput:[NSString stringWithFormat:@"Testing HTTPS with url \'%@\'\n\n", testUrl]];
    } else {
        testUrl = HTTPS_TEST_DEFAULT_URL;
        [self.urlText setText:testUrl];
        [self appendOutput:[NSString stringWithFormat:@"Testing HTTPS with default url \'%@\'\n\n", testUrl]];
    }
    
    // SPLITTING COMMAND ARGUMENTS
    NSString* ffmpegCommand = [NSString stringWithFormat:@"-hide_banner -i %@", testUrl];

    [self appendOutput:[NSString stringWithFormat:@"ffmpeg process started with arguments\n\'%@\'\n\n", ffmpegCommand]];

    // Execute
    int result = [MobileFFmpeg execute: ffmpegCommand];

    [self appendOutput:[NSString stringWithFormat:@"ffmpeg process exited with rc %d\n\n", result]];
}

- (void)showTooltip {
    [tooltip showAnimated:YES forView:self.getInfoButton withinSuperView:self.view];
}

- (void)hideTooltip {
    [tooltip dismissWithCompletion:nil];
}

- (void)clearOutput {
    [[self outputText] setText:@""];
}

- (void)appendOutput:(NSString*) message {
    self.outputText.text = [self.outputText.text stringByAppendingString:message];
}

@end
