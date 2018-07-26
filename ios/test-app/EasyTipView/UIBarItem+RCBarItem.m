//
//  UIBarItem+RCBarItem.m
//  EasyTips
//
//  Created by Nitish Makhija on 16/12/16.
//  Copyright © 2016 Roadcast. All rights reserved.
//

#import "UIBarItem+RCBarItem.h"

@implementation UIBarItem (RCBarItem)

- (UIView *)view {
    UIBarButtonItem *item = (UIBarButtonItem *)self;
    UIView *customView = item.customView;
    if (item && customView) {
        return customView;
    }
    
    return (UIView *)[self valueForKey:@"view"];
}

@end
