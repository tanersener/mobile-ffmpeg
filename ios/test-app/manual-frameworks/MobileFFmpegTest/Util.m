//
// Util.m
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

#import "Util.h"

@implementation Util

+ (void)applyButtonStyle: (UIButton*) button {
    button.tintColor = [UIColor whiteColor];
    [button setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    button.layer.backgroundColor = [UIColor colorWithDisplayP3Red:46.0/256 green:204.0/256 blue:113.0/256 alpha:1.0].CGColor;
    button.layer.borderWidth = 1.0f;
    button.layer.borderColor = [UIColor colorWithDisplayP3Red:39.0/256 green:174.0/256 blue:96.0/256 alpha:1.0].CGColor;
    button.layer.cornerRadius = 5.0f;
}

+ (void)applyEditTextStyle: (UITextField*) textField {
    textField.layer.borderWidth = 1.0f;
    textField.layer.borderColor = [UIColor colorWithDisplayP3Red:52.0/256 green:152.0/256 blue:219.0/256 alpha:1.0].CGColor;
    textField.layer.cornerRadius = 5.0f;
}

+ (void)applyHeaderStyle: (UILabel*) label {
    label.layer.borderWidth = 1.0f;
    label.layer.borderColor = [UIColor colorWithDisplayP3Red:231.0/256 green:76.0/256 blue:60.0/256 alpha:1.0].CGColor;
    label.layer.cornerRadius = 5.0f;
}

+ (void)applyOutputTextStyle: (UITextView*) textView {
    textView.layer.backgroundColor = [UIColor colorWithDisplayP3Red:241.0/256 green:196.0/256 blue:15.0/256 alpha:1.0].CGColor;
    textView.layer.borderWidth = 1.0f;
    textView.layer.borderColor = [UIColor colorWithDisplayP3Red:243.0/256 green:156.0/256 blue:18.0/256 alpha:1.0].CGColor;
    textView.layer.cornerRadius = 5.0f;
}

+ (void)applyPickerViewStyle: (UIPickerView*) pickerView {
    pickerView.layer.backgroundColor = [UIColor colorWithDisplayP3Red:155.0/256 green:89.0/256 blue:182.0/256 alpha:1.0].CGColor;
    pickerView.layer.borderWidth = 1.0f;
    pickerView.layer.borderColor = [UIColor colorWithDisplayP3Red:142.0/256 green:68.0/256 blue:173.0/256 alpha:1.0].CGColor;
    pickerView.layer.cornerRadius = 5.0f;
}

+ (void)applyVideoPlayerFrameStyle: (UILabel*) playerFrame {
    playerFrame.layer.backgroundColor = [UIColor colorWithDisplayP3Red:236.0/256 green:240.0/256 blue:241.0/256 alpha:1.0].CGColor;
    playerFrame.layer.borderWidth = 1.0f;
    playerFrame.layer.borderColor = [UIColor colorWithDisplayP3Red:185.0/256 green:195.0/256 blue:199.0/256 alpha:1.0].CGColor;
    playerFrame.layer.cornerRadius = 5.0f;
}

+ (void)alert: (UIViewController*)controller withTitle:(NSString*)title message:(NSString*)message andButtonText:(NSString*)buttonText {
    UIAlertController* alert = [UIAlertController
                                alertControllerWithTitle:title
                                message:message
                                preferredStyle:UIAlertControllerStyleAlert];
    
    UIAlertAction* defaultAction = [UIAlertAction
                                    actionWithTitle:buttonText style:UIAlertActionStyleDefault
                                    handler:^(UIAlertAction * action) {}];
    
    [alert addAction:defaultAction];
    [controller presentViewController:alert animated:YES completion:nil];
}

@end
