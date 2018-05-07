//
//  FirstViewController.m
//  MobileFFmpegTest
//
//  Created by Taner Sener on 6.05.2018.
//  Copyright © 2018 ARTHENICA. All rights reserved.
//

#import "FirstViewController.h"

#import <mobileffmpeg/archdetect.h>
#import <mobileffmpeg/mobileffmpeg.h>

@interface FirstViewController ()

@end

@implementation FirstViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
}


- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)runAction:(id)sender {
    NSString *parameter = @"-version";
    char** cc = malloc(sizeof(char*));
    cc[0] = (char *) [parameter UTF8String];

    const char* arch = getArch();
    
    NSLog(@"Parameter is %s\n", cc[0]);
    NSLog(@"Architecture is %s\n", arch);
    
    int result = mobileffmpeg_execute(1, cc);
    
    NSLog(@"Result is %d\n", result);
    
    free(cc);
}

@end
