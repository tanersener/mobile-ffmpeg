//
//  RCEasyTipViewIDGenerator.m
//  EasyTips
//
//  Created by Nitish Makhija on 20/12/16.
//  Copyright © 2016 Roadcast. All rights reserved.

#import "RCEasyTipViewIDGenerator.h"

static NSInteger nextViewID = 0;

@implementation RCEasyTipViewIDGenerator

+ (NSInteger)getUniqueViewID {
    nextViewID++;
    return nextViewID;
}

@end
