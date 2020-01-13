//
// CommandViewController.m
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

#import <mobileffmpeg/MobileFFmpegConfig.h>
#import <mobileffmpeg/MobileFFmpeg.h>
#import "CommandViewController.h"

@interface CommandViewController ()

@property (strong, nonatomic) IBOutlet UILabel *header;
@property (strong, nonatomic) IBOutlet UITextField *commandText;
@property (strong, nonatomic) IBOutlet UIButton *runFFmpegButton;
@property (strong, nonatomic) IBOutlet UIButton *runFFprobeButton;
@property (strong, nonatomic) IBOutlet UITextView *outputText;

@end

@implementation CommandViewController {
}

- (void)viewDidLoad {
    [super viewDidLoad];

    // STYLE UPDATE
    [Util applyEditTextStyle: self.commandText];
    [Util applyButtonStyle: self.runFFmpegButton];
    [Util applyButtonStyle: self.runFFprobeButton];
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

- (IBAction)runFFmpegAction:(id)sender {
    [self clearOutput];
    
    [[self commandText] endEditing:TRUE];
    
    NSString *ffmpegCommand = [NSString stringWithFormat:@"-hide_banner %@", [[self commandText] text]];
    
    NSLog(@"Testing FFmpeg COMMAND synchronously.\n");
    
    NSLog(@"FFmpeg process started with arguments\n\'%@\'\n", ffmpegCommand);
    
    // EXECUTE
    int result = [MobileFFmpeg execute:ffmpegCommand];
    
    NSLog(@"FFmpeg process exited with rc %d\n", result);

    if (result != RETURN_CODE_SUCCESS) {
        [Util alert:self withTitle:@"Error" message:@"Command failed. Please check output for the details." andButtonText:@"OK"];
    }
}

- (IBAction)runFFprobeAction:(id)sender {
    [self clearOutput];
    
    [[self commandText] endEditing:TRUE];
    
    NSString *ffprobeCommand = [NSString stringWithFormat:@"-hide_banner %@", [[self commandText] text]];
    
    NSLog(@"Testing FFprobe COMMAND synchronously.\n");
    
    NSLog(@"FFprobe process started with arguments\n\'%@\'\n", ffprobeCommand);
    
    // EXECUTE
    int result = [MobileFFprobe execute:ffprobeCommand];
    
    NSLog(@"FFprobe process exited with rc %d\n", result);

    if (result != RETURN_CODE_SUCCESS) {
        [Util alert:self withTitle:@"Error" message:@"Command failed. Please check output for the details." andButtonText:@"OK"];
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
        // NSRange bottom = NSMakeRange(self.outputText.text.length - 1, 1);
        // [self.outputText scrollRangeToVisible:bottom];
    }
}

@end
