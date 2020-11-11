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

#include <atomic>
#include <memory>
#include <vector>

#include "RenderDevice.h"
#include "DeviceContext.h"
#include "RefCntAutoPtr.hpp"
#include "SwapChain.h"
#include "GraphicsTypesOutputInserters.hpp"
#include "NativeWindow.h"

#include "gtest/gtest.h"

namespace Diligent
{

namespace Testing
{

class TestingEnvironment : public ::testing::Environment
{
public:
    struct CreateInfo
    {
        RENDER_DEVICE_TYPE deviceType  = RENDER_DEVICE_TYPE_UNDEFINED;
        ADAPTER_TYPE       AdapterType = ADAPTER_TYPE_UNKNOWN;
        Uint32             AdapterId   = DEFAULT_ADAPTER_ID;

        bool ForceNonSeparablePrograms = false;
    };
    TestingEnvironment(const CreateInfo& CI, const SwapChainDesc& SCDesc);

    ~TestingEnvironment() override;

    // Override this to define how to set up the environment.
    void SetUp() override final;

    // Override this to define how to tear down the environment.
    void TearDown() override final;

    virtual void Reset();

    void ReleaseResources();

    class ScopedReset
    {
    public:
        ScopedReset() = default;
        ~ScopedReset()
        {
            if (m_pTheEnvironment)
                m_pTheEnvironment->Reset();
        }
    };

    class ScopedReleaseResources
    {
    public:
        ScopedReleaseResources() = default;
        ~ScopedReleaseResources()
        {
            if (m_pTheEnvironment)
                m_pTheEnvironment->ReleaseResources();
        }
    };

    IRenderDevice*  GetDevice() { return m_pDevice; }
    IDeviceContext* GetDeviceContext() { return m_pDeviceContext; }
    ISwapChain*     GetSwapChain() { return m_pSwapChain; }

    static TestingEnvironment* GetInstance() { return m_pTheEnvironment; }

    RefCntAutoPtr<ITexture> CreateTexture(const char* Name, TEXTURE_FORMAT Fmt, BIND_FLAGS BindFlags, Uint32 Width, Uint32 Height);

    static void SetErrorAllowance(int NumErrorsToAllow, const char* InfoMessage = nullptr);

    void            SetDefaultCompiler(SHADER_COMPILER compiler);
    SHADER_COMPILER GetDefaultCompiler(SHADER_SOURCE_LANGUAGE lang) const;

protected:
    NativeWindow CreateNativeWindow();

    static void MessageCallback(DEBUG_MESSAGE_SEVERITY Severity,
                                const Char*            Message,
                                const char*            Function,
                                const char*            File,
                                int                    Line);

    static Uint32 FindAdapater(const std::vector<GraphicsAdapterInfo>& Adapters,
                               ADAPTER_TYPE                            AdapterType,
                               Uint32                                  AdapterId);

    const RENDER_DEVICE_TYPE m_DeviceType;

    // Any platform-specific data (e.g. window handle) that should
    // be cleaned-up when the testing environment object is destroyed.
    struct PlatformData
    {
        virtual ~PlatformData() {}
    };
    std::unique_ptr<PlatformData> m_pPlatformData;

    static TestingEnvironment* m_pTheEnvironment;

    RefCntAutoPtr<IRenderDevice>  m_pDevice;
    RefCntAutoPtr<IDeviceContext> m_pDeviceContext;
    RefCntAutoPtr<ISwapChain>     m_pSwapChain;
    SHADER_COMPILER               m_ShaderCompiler = SHADER_COMPILER_DEFAULT;

    static std::atomic_int m_NumAllowedErrors;
};

} // namespace Testing

} // namespace Diligent
