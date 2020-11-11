/*
 *  Copyright 2019-2020 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
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

#pragma once

#ifndef NOMINMAX
#    define NOMINMAX
#endif
#include <Windows.h>

#include "AppBase.hpp"

namespace Diligent
{

/// Base class for Win32 applications.
class Win32AppBase : public AppBase
{
public:
    /// Called by the framework after the window has been created.

    /// \param [in] hWnd         - Window handle.
    /// \param [in] WindowWidth  - Window width.
    /// \param [in] WindowHeight - Window height.
    ///
    /// \remarks An application may override AppBase::GetDesiredInitialWindowSize
    ///          method to specify desired initial window size.
    virtual void OnWindowCreated(HWND hWnd,
                                 LONG WindowWidth,
                                 LONG WindowHeight) = 0;

    /// Handles Win32 message

    /// An application may override this method to implement its
    /// windows message processing routine.
    /// \param [in] hWnd     - Window handle.
    /// \param [in] message  - Window message.
    /// \param [in] wParam   - Window message wparam.
    /// \param [in] lParam   - Window message lparam.
    virtual LRESULT HandleWin32Message(HWND   hWnd,
                                       UINT   message,
                                       WPARAM wParam,
                                       LPARAM lParam)
    {
        return 0;
    }
};

} // namespace Diligent
