//
// TabBarController.m
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

#import "TabBarController.h"
#import "CommandViewController.h"
#import "VideoViewController.h"
#import "HttpsViewController.h"
#import "AudioViewController.h"
#import "SubtitleViewController.h"
#import "VidStabViewController.h"
#import "PipeViewController.h"

@interface TabBarController () <UITabBarControllerDelegate>

@end

@implementation TabBarController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    self.delegate = self;
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}

- (void)tabBarController: (UITabBarController *)tabBarController didSelectViewController: (UIViewController *)viewController {
    
    if ([viewController isKindOfClass:[CommandViewController class]]) {
        CommandViewController* commandView = (CommandViewController*)viewController;
        [commandView setActive];
    } else if ([viewController isKindOfClass:[VideoViewController class]]) {
        VideoViewController* videoView = (VideoViewController*)viewController;
        [videoView setActive];
    } else if ([viewController isKindOfClass:[HttpsViewController class]]) {
        HttpsViewController* httpsView = (HttpsViewController*)viewController;
        [httpsView setActive];
    } else if ([viewController isKindOfClass:[AudioViewController class]]) {
        AudioViewController* audioView = (AudioViewController*)viewController;
        [audioView setActive];
    } else if ([viewController isKindOfClass:[SubtitleViewController class]]) {
        SubtitleViewController* subtitleView = (SubtitleViewController*)viewController;
        [subtitleView setActive];
    } else if ([viewController isKindOfClass:[VidStabViewController class]]) {
        VidStabViewController* vidStabView = (VidStabViewController*)viewController;
        [vidStabView setActive];
    } else if ([viewController isKindOfClass:[PipeViewController class]]) {
        PipeViewController* pipeView = (PipeViewController*)viewController;
        [pipeView setActive];
    }
}

@end
