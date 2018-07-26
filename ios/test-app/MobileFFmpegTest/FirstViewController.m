//
// FirstViewController.m
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

#import "FirstViewController.h"
#import <mobileffmpeg/Log.h>
#import <mobileffmpeg/MobileFFmpeg.h>

@interface FirstViewController ()

@property (strong, nonatomic) IBOutlet UITextField *commandText;
@property (strong, nonatomic) IBOutlet UITextView *runOutput;

@end

@implementation FirstViewController

- (void)viewDidLoad {
    [super viewDidLoad];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}

- (IBAction)runAction:(id)sender {
    [Log enableRedirection];

    [Log setLevel:AV_LOG_WARNING];
    
    NSString *command = [[self commandText] text];

    [self clearLog];

    [self appendLog:[NSString stringWithFormat:@"Running synchronously with arguments '%@'\n", command]];

    // Execute
    int result = [MobileFFmpeg execute: command];

    [self appendLog:[NSString stringWithFormat:@"Process exited with rc %d\n", result]];
}

- (IBAction)runAsyncAction:(id)sender {
    [Log disableRedirection];

    [Log setLevel:AV_LOG_INFO];
    
    NSString *command = [[self commandText] text];
    
    dispatch_async(dispatch_get_main_queue(), ^{

        [self clearLog];
        
        [self appendLog:[NSString stringWithFormat:@"Running asynchronously with arguments '%@'\n", command]];
        
        // Execute
        int result = [MobileFFmpeg execute: command];
        
        [self appendLog:[NSString stringWithFormat:@"Async process exited with rc %d\n", result]];
    });
}

- (void)clearLog {
    [[self runOutput] setText:@""];
}

- (void)appendLog:(NSString*) logMessage {
    self.runOutput.text = [self.runOutput.text stringByAppendingString:logMessage];
}


@end
