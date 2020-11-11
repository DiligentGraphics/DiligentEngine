/*     Copyright 2015-2019 Egor Yusov
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF ANY PROPRIETARY RIGHTS.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#import "ModeSelectionViewController.h"
#import "GLView.h"
#import "MetalView.h"
#import "ViewController.h"


@implementation ModeSelectionViewController
{
}

- (void) setWindowTitle:(NSString*) title
{
    NSWindow* mainWindow = [[NSApplication sharedApplication]mainWindow];
    [mainWindow setTitle:title];
}

- (void) terminateApp:(NSString*) error
{
    NSAlert *alert = [[NSAlert alloc] init];
    [alert addButtonWithTitle:@"OK"];
    [alert setMessageText:@"Failed to start the application"];
    [alert setInformativeText:error];
    [alert setAlertStyle:NSAlertStyleCritical];
    [alert runModal];
    [NSApp terminate:self];
}

- (IBAction)goOpenGL:(id)sender
{
    ViewController* glViewController = [self.storyboard instantiateControllerWithIdentifier:@"GLViewControllerID"];
    self.view.window.contentViewController = glViewController;

    GLView* glView = (GLView*)[glViewController view];
    NSString* error = [glView getError];
    if(error != nil)
    {
        [self terminateApp:error];
    }

    NSString* name =  [glView getAppName];
    [self setWindowTitle:name];
}

- (IBAction)goVulkan:(id)sender
{
    ViewController* metalViewController = [self.storyboard instantiateControllerWithIdentifier:@"MetalViewControllerID"];
    self.view.window.contentViewController = metalViewController;

    MetalView* mtlView = (MetalView*)[metalViewController view];
    NSString* error = [mtlView getError];
    if(error != nil)
    {
        [self terminateApp:error];
    }

    NSString* name =  [mtlView getAppName];
    [self setWindowTitle:name];
}

@end
