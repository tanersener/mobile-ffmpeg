//
//  UIView+RCEssentials.m
//  RoadCast
//
//  Created by Nitish Makhija on 16/05/16.
//  Copyright © 2016 RoadCast. All rights reserved.
//

#import "UIView+RCEssentials.h"

@implementation UIView (RCEssentials)

- (id)createAndAddSubView:(Class)modelClass {
    id temporaryView = [modelClass new];
    [self insertSubview:(UIView *)temporaryView atIndex:self.subviews.count];
    return temporaryView;
}

- (BOOL)hasSuperView:(UIView *)superView {
    return [self view:self HasSuperView:superView];
}

- (BOOL)view:(UIView *)view HasSuperView:(UIView *)superView {
    UIView *sView = view.superview;
    if (sView) {
        if ([sView isEqual:superView]) {
            return YES;
        } else {
            return [self view:sView HasSuperView:superView];
        }
    } else {
        return NO;
    }
}

@end
