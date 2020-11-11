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

namespace Diligent
{

/// Base class for native applications. Platform-specific classes
/// such as Win32AppBase, LinuxAppBase are inherited from AppBase.
class AppBase
{
public:
    /// Golden image capture mode
    enum class GoldenImageMode
    {
        /// Gloden image processing is disabled
        None = 0,

        /// Capture the golden image. In this mode, the
        /// application renders one frame, captures it as
        /// a golden image and exits.
        Capture,

        /// Compare the golden image. In this mode, the application renders
        /// one frame, compares it with the golden image and exists.
        /// Zero exit code indicates that the frame is identical to the golden image.
        /// The non-zero code indicates the number of pixels that differ.
        Compare,

        /// Compare the golden image as in Compare mode, and then update
        /// it as in Capture mode.
        CompareUpdate
    };

    virtual ~AppBase() {}


    /// Processes the command line arguments.

    /// The method is called by the framework to let the application process
    /// the command line arguments. This method is called before any other method is called.
    /// \param [in] CmdLine - The command line string.
    virtual void ProcessCommandLine(const char* CmdLine) = 0;


    /// Returns the application tile.

    /// An application must override this method to define the application title.
    /// \return     The application title
    virtual const char* GetAppTitle() const = 0;


    /// Updates the application state.

    /// This method is called by the framework to let the application perform
    /// the required update operations.
    /// \param [in] CurrTime    - Current time, i.e. the time elapsed since the application started.
    /// \param [in] ElapsedTime - The time elapsed since the previous frame update.
    virtual void Update(double CurrTime, double ElapsedTime){};


    /// Renders the frame.

    /// An application must override this method to perform operations
    /// required to render the frame.
    virtual void Render() = 0;


    /// Presents the frame.

    /// An application must override this method to perform operations
    /// required to present the rendered frame on the screen.
    virtual void Present() = 0;


    /// Called when the window resizes.

    /// An application must override this method to perform operations
    /// required to resize the window.
    /// \param [in] width  - New window width
    /// \param [in] height - New window height
    virtual void WindowResize(int width, int height) = 0;


    /// Called by the framework to request the desired initial window size.

    /// This method is called before the platform-specific window is created.
    /// An application may override this method to speciy required initial
    /// window width and height.
    virtual void GetDesiredInitialWindowSize(int& width, int& height)
    {
        width  = 0;
        height = 0;
    }


    /// Returns the golden image mode, see Diligent::AppBase::GoldenImageMode.
    virtual GoldenImageMode GetGoldenImageMode() const
    {
        return GoldenImageMode::None;
    }


    /// Returns the exit code.

    /// An application may override this method to
    /// return a specific exit code.
    virtual int GetExitCode() const
    {
        return 0;
    }

    /// Returns true if the app is initialized and ready to run
    virtual bool IsReady() const
    {
        return false;
    }
};

} // namespace Diligent
