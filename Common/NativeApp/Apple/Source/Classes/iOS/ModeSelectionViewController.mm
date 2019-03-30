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
#import "BaseView.h"
#import "AppViewBase.h"

@implementation ModeSelectionViewController
{
}

-(void)selectViewController:(NSString*)controllerID
{
    auto animating = ((BaseView*)self.view).animating;
    
    UIViewController* viewController = [self.storyboard instantiateViewControllerWithIdentifier:controllerID];
    self.view.window.rootViewController = viewController;
    
    NSString *error = [(AppViewBase*)viewController.view getError];
    if(error != nil)
    {
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Failed to start the application" message:error delegate:self cancelButtonTitle:@"OK" otherButtonTitles:@"Whatever", nil];
        [alert show];
    }
    
    if(animating)
    {
        [(BaseView*)viewController.view startAnimation];
    }
}

- (IBAction)goOpenGLES:(id)sender
{
    [self selectViewController:@"EAGLViewControllerID"];
}

- (IBAction)goVulkan:(id)sender
{
#if VULKAN_SUPPORTED
    [self selectViewController:@"MetalViewControllerID"];
#endif
}

@end
