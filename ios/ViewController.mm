/*
 *  DemoViewController.mm
 *
 *  Copyright (c) 2016-2017 The Brenwill Workshop Ltd.
 *  This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#import "ViewController.h"

#include "Application.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/InputManager.hpp"

std::string MFA::Path::GetAssetPath() {
    return [NSBundle.mainBundle.resourcePath stringByAppendingString: @"/"].UTF8String;
}

#pragma mark -
#pragma mark ViewController

@implementation ViewController {
    Application mApplication;
    CADisplayLink * mDisplayLink;
    bool mViewHasAppeared;
    CFTimeInterval mPreviousFrameTime;
}

/** Since this is a single-view app, init Vulkan when the view is loaded. */
-(void) viewDidLoad {
    [super viewDidLoad];
    
    mPreviousFrameTime = 0.0f;
    
    MFA_LOG_INFO("IOS: Main screen info: NativeScale: %f, Width: %f, Height: %f"
                 , UIScreen.mainScreen.nativeScale, UIScreen.mainScreen.bounds.size.width, UIScreen.mainScreen.bounds.size.height);
    
    
    self.view.contentScaleFactor = UIScreen.mainScreen.nativeScale;
    
    mApplication.SetView((__bridge void *)(self.view));
    mApplication.Init();
    
    static constexpr uint32_t TargetFPS = 60;
    mDisplayLink = [CADisplayLink displayLinkWithTarget: self selector: @selector(renderFrame)];
    [mDisplayLink setPreferredFramesPerSecond: TargetFPS];
    [mDisplayLink addToRunLoop: NSRunLoop.currentRunLoop forMode: NSDefaultRunLoopMode];

    // Setup tap gesture to toggle virtual keyboard
    UITapGestureRecognizer* tapSelector = [[UITapGestureRecognizer alloc]
                                           initWithTarget: self action: @selector(handleTapGesture:)];
    tapSelector.numberOfTapsRequired = 1;
    tapSelector.cancelsTouchesInView = YES;
    [self.view addGestureRecognizer: tapSelector];

    mViewHasAppeared = NO;
}

-(void) viewDidAppear: (BOOL) animated {
    [super viewDidAppear: animated];
    mViewHasAppeared = YES;
}

-(BOOL) canBecomeFirstResponder { return mViewHasAppeared; }

-(void) renderFrame {
    static constexpr float MinimumDeltaTimeInSec = 1.0f / 30.0f;
    auto currentFrameTime = [mDisplayLink timestamp];
    float deltaTime = std::fminf(currentFrameTime - mPreviousFrameTime, MinimumDeltaTimeInSec);
    mPreviousFrameTime = currentFrameTime;
    mApplication.RenderFrame(deltaTime);
}

-(void) dealloc {
    mApplication.Shutdown();
}

// Toggle the display of the virtual keyboard
-(void) toggleKeyboard {
    if (self.isFirstResponder) {
        [self resignFirstResponder];
    } else {
        [self becomeFirstResponder];
    }
}

// Display and hide the keyboard by tapping on the view
-(void) handleTapGesture: (UITapGestureRecognizer*) gestureRecognizer {
//    if (gestureRecognizer.state == UIGestureRecognizerStateEnded) {
//        [self toggleKeyboard];
//    }
    // TODO handle gesture
//    [touch ]
}

- (void) touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    UITouch *touch = [[event allTouches] anyObject];
    CGPoint location = [touch preciseLocationInView: self.view];
    MFA::InputManager::UpdateTouchState(true, true, location.x * UIScreen.mainScreen.nativeScale, location.y * UIScreen.mainScreen.nativeScale);
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    UITouch *touch = [[event allTouches] anyObject];
    CGPoint location = [touch locationInView: self.view];
//    MFA_LOG_INFO("TouchMoved: %f %f", location.x, location.y);
    MFA::InputManager::UpdateTouchState(true, true, location.x * UIScreen.mainScreen.nativeScale, location.y * UIScreen.mainScreen.nativeScale);
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    UITouch *touch = [[event allTouches] anyObject];
    CGPoint location = [touch locationInView: self.view];
//    MFA_LOG_INFO("TouchEnded: %f %f", location.x, location.y);
    MFA::InputManager::UpdateTouchState(false, true, location.x * UIScreen.mainScreen.nativeScale, location.y * UIScreen.mainScreen.nativeScale);
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    UITouch *touch = [[event allTouches] anyObject];
    CGPoint location = [touch locationInView: self.view];
//    MFA_LOG_INFO("TouchCancelled: %f %f", location.x, location.y);
    MFA::InputManager::UpdateTouchState(false, false, location.x * UIScreen.mainScreen.nativeScale, location.y * UIScreen.mainScreen.nativeScale);
}

// Handle keyboard input
-(void) handleKeyboardInput: (unichar) keycode {
//    _mvkExample->keyPressed(keycode);
}


#pragma mark UIKeyInput methods

// Returns whether text is available
-(BOOL) hasText { return YES; }

// A key on the keyboard has been pressed.
-(void) insertText: (NSString*) text {
//    unichar keycode = (text.length > 0) ? [text characterAtIndex: 0] : 0;
//    [self handleKeyboardInput: keycode];
}

// The delete backward key has been pressed.
-(void) deleteBackward {
//    [self handleKeyboardInput: 0x33];
}


@end


#pragma mark -
#pragma mark RootView

@implementation RootView

/** Returns a Metal-compatible layer. */
+(Class) layerClass { return [CAMetalLayer class]; }

@end

