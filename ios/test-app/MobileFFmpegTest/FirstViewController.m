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

#import <mobileffmpeg/log.h>
#import <mobileffmpeg/mobileffmpeg.h>

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
    
    // SPLITTING COMMAND ARGUMENTS
    NSArray* commandArray = [[[self commandText] text] componentsSeparatedByString:@" "];
    char **arguments = (char **)malloc(sizeof(char*) * ([commandArray count]));
    for (int i=0; i < [commandArray count]; i++) {
        NSString *argument = [commandArray objectAtIndex:i];
        arguments[i] = (char *) [argument UTF8String];
    }

    [self clearLog];

    [self appendLog:[NSString stringWithFormat:@"Running synchronously with arguments '%s'\n", self.commandText.text.UTF8String]];

    // Executing
    int result = mobileffmpeg_execute((int) [commandArray count], arguments);

    [self appendLog:[NSString stringWithFormat:@"Process exited with rc %d\n", result]];

    // CLEANING ARGUMENTS
    free(arguments);
}

- (IBAction)runAsyncAction:(id)sender {
    
    dispatch_async(dispatch_get_main_queue(), ^{

        // SPLITTING COMMAND ARGUMENTS
        NSArray* commandArray = [[[self commandText] text] componentsSeparatedByString:@" "];
        char **arguments = (char **)malloc(sizeof(char*) * ([commandArray count]));
        for (int i=0; i < [commandArray count]; i++) {
            NSString *argument = [commandArray objectAtIndex:i];
            arguments[i] = (char *) [argument UTF8String];
        }
        
        [self clearLog];
        
        [self appendLog:[NSString stringWithFormat:@"Running asynchronously with arguments '%s'\n", self.commandText.text.UTF8String]];
        
        // Executing
        int result = mobileffmpeg_execute((int) [commandArray count], arguments);
        
        [self appendLog:[NSString stringWithFormat:@"Async process exited with rc %d\n", result]];
        
        // CLEANING ARGUMENTS
        free(arguments);
    });
}

- (void)clearLog {
    [[self runOutput] setText:@""];
}

- (void)appendLog:(NSString*) logMessage {
    self.runOutput.text = [self.runOutput.text stringByAppendingString:logMessage];
}


@end
