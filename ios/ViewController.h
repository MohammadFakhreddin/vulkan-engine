/*
 *  DemoViewController.h
 *
 *  Copyright (c) 2016-2017 The Brenwill Workshop Ltd.
 *  This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#import <UIKit/UIKit.h>


#pragma mark -
#pragma mark ViewController

/** The main view controller for the demo storyboard. */
@interface ViewController : UIViewController <UIKeyInput>
@end


#pragma mark -
#pragma mark RootView

/** The Metal-compatibile view for the demo Storyboard. */
@interface RootView : UIView
@end

