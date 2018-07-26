//
//  RCEasyTipView.h
//  EasyTips
//
//  Created by Nitish Makhija on 16/12/16.
//  Copyright © 2016 Roadcast. All rights reserved.

#import <UIKit/UIKit.h>

typedef NS_ENUM(NSInteger, ArrowPosition){
    Any = 0,
    Top,
    Bottom,
    Right,
    Left
};

@class RCEasyTipDrawing;
@class RCEasyTipPositioning;
@class RCEasyTipAnimating;

@interface RCEasyTipPreferences : NSObject

@property (nonatomic, strong) RCEasyTipDrawing *drawing;
@property (nonatomic, strong) RCEasyTipPositioning *positioning;
@property (nonatomic, strong) RCEasyTipAnimating *animating;
/**
 Defaults to NO.
 */
@property (nonatomic, assign) BOOL shouldDismissOnTouchOutside;


- (instancetype)initWithDefaultPreferences;

@end

@interface RCEasyTipDrawing : RCEasyTipPreferences

@property (nonatomic, assign) ArrowPosition arrowPostion;
@property (nonatomic, assign) CGFloat cornerRadius;
@property (nonatomic, assign) CGFloat arrowHeight;
@property (nonatomic, assign) CGFloat arrowWidth;
@property (nonatomic, strong) UIColor *foregroundColor;
@property (nonatomic, strong) UIColor *backgroundColor;
@property (nonatomic, assign) NSTextAlignment textAlignment;
@property (nonatomic, strong) UIColor *borderColor;
@property (nonatomic, assign) CGFloat borderWidth;
@property (nonatomic, strong) UIFont *font;

@end

@interface RCEasyTipPositioning : RCEasyTipPreferences

@property (nonatomic, assign) CGFloat bubbleHInset;
@property (nonatomic, assign) CGFloat bubbleVInset;
@property (nonatomic, assign) CGFloat textHInset;
@property (nonatomic, assign) CGFloat textVInset;
@property (nonatomic, assign) CGFloat maxWidth;

@end

@interface RCEasyTipAnimating : RCEasyTipPreferences

@property (nonatomic, assign) CGAffineTransform dismissTransform;
@property (nonatomic, assign) CGAffineTransform showInitialTransform;
@property (nonatomic, assign) CGAffineTransform showFinalTransform;
@property (nonatomic, assign) CGFloat springDamping;
@property (nonatomic, assign) CGFloat springVelocity;
@property (nonatomic, assign) CGFloat showInitialAlpha;
@property (nonatomic, assign) CGFloat dismissFinalAlpha;
@property (nonatomic, assign) NSTimeInterval showDuration;
@property (nonatomic, assign) NSTimeInterval dismissDuration;

@end

@protocol RCEasyTipViewDelegate;

@interface RCEasyTipView : UIView

/**
 Unique ID for Each RCEasyTipView
 */
@property (readonly, assign) NSInteger ID;

/**
 Text to be displayed inside RCEasyTipView
 */
@property (nonatomic, strong) NSString *text;

/**
 Delegate for RCEasyTipView
 */
@property (nonatomic, weak) id<RCEasyTipViewDelegate>delegate;

/**
 Custom RCEasyTipPreferences initializer for RCEasyTipView
 
 @param preferences RCEasyTipPreferences object full customized with provided properties
 @return instancetype for RCEasyTipView
 */
- (instancetype)initWithPreferences:(RCEasyTipPreferences *)preferences;

/**
 Default initializer for RCEasyTipView
 
 @param text Text to be displayed inside RCEasyTipView
 @return instancetype for RCEasyTipView
 */
- (instancetype)initWithText:(NSString *)text;

/**
 Custom RCEasyTipPreferences initializer for RCEasyTipView with text to displayed inside RCEasyTipView
 
 @param preferences RCEasyTipPreferences object full customized with provided properties
 @param text Text to be displayed inside RCEasyTipView
 @return instancetype for RCEasyTipView
 */
- (instancetype)initWithPreferences:(RCEasyTipPreferences *)preferences andText:(NSString *)text;

/**
 Presents an EasyTipView pointing to a particular UIBarItem instance within the specified superview
 
 @param animated Pass true to animate the presentation.
 @param item The UIBarButtonItem or UITabBarItem instance which the EasyTipView will be pointing to.
 @param superView   A view which is part of the UIBarButtonItem instances superview hierarchy. Ignore this parameter in order to display the EasyTipView within the main window.
 */
- (void)showAnimated:(BOOL)animated forItem:(UIBarItem *)item withinSuperView:(UIView *)superView;

/**
 Presents an EasyTipView pointing to a particular UIView instance within the specified superview
 
 @param animated  Pass true to animate the presentation.
 @param view      The UIView instance which the EasyTipView will be pointing to.
 @param superView A view which is part of the UIView instances superview hierarchy. Ignore this parameter in order to display the EasyTipView within the main window.
 */
- (void)showAnimated:(BOOL)animated forView:(UIView *)view withinSuperView:(UIView *)superView;

/**
 Dismisses the EasyTipView
 
 @param completionBlock Completion block to be executed after the EasyTipView is dismissed.
 */
- (void)dismissWithCompletion:(void (^)())completionBlock;
@end

@protocol RCEasyTipViewDelegate <NSObject>
@optional

- (void)willShowTip:(RCEasyTipView *)tipView;
- (void)didShowTip:(RCEasyTipView *)tipView;

- (void)willDismissTip:(RCEasyTipView *)tipView;
- (void)didDismissTip:(RCEasyTipView *)tipView;

@end
