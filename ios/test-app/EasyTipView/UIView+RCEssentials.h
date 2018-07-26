//
//  UIView+RCEssentials.h
//  RoadCast
//
//  Created by Nitish Makhija on 16/05/16.
//  Copyright © 2016 RoadCast. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface UIView (RCEssentials)

- (id)createAndAddSubView:(Class)modelClass;

- (BOOL)hasSuperView:(UIView *)superView;

@end
