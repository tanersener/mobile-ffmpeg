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
#import "FirstViewController.h"
#import "SecondViewController.h"
#import "HttpsViewController.h"

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
    
    if ([viewController isKindOfClass:[FirstViewController class]]) {
    } else if ([viewController isKindOfClass:[SecondViewController class]]) {
    } else if ([viewController isKindOfClass:[HttpsViewController class]]) {
        HttpsViewController* httpsView = (HttpsViewController*)viewController;
        [httpsView hideTooltip];
        [httpsView showTooltip];
    }
}

@end
