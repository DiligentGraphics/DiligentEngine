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

#include <iostream>
#include <string>

#include "gtest/gtest.h"
#include "TestingEnvironment.hpp"

#if PLATFORM_WIN32
#    include <crtdbg.h>
#endif


namespace Diligent
{

namespace Testing
{

#if D3D11_SUPPORTED
TestingEnvironment* CreateTestingEnvironmentD3D11(const TestingEnvironment::CreateInfo& CI, const SwapChainDesc& SCDesc);
#endif

#if D3D12_SUPPORTED
TestingEnvironment* CreateTestingEnvironmentD3D12(const TestingEnvironment::CreateInfo& CI, const SwapChainDesc& SCDesc);
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
TestingEnvironment* CreateTestingEnvironmentGL(const TestingEnvironment::CreateInfo& CI, const SwapChainDesc& SCDesc);
#endif

#if VULKAN_SUPPORTED
TestingEnvironment* CreateTestingEnvironmentVk(const TestingEnvironment::CreateInfo& CI, const SwapChainDesc& SCDesc);
#endif

#if METAL_SUPPORTED
TestingEnvironment* CreateTestingEnvironmentMtl(const TestingEnvironment::CreateInfo& CI, const SwapChainDesc& SCDesc);
#endif

} // namespace Testing

} // namespace Diligent


using namespace Diligent;
using namespace Diligent::Testing;

int main(int argc, char** argv)
{
#if PLATFORM_WIN32
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    ::testing::InitGoogleTest(&argc, argv);

    TestingEnvironment::CreateInfo TestEnvCI;
    SHADER_COMPILER                ShCompiler = SHADER_COMPILER_DEFAULT;
    for (int i = 1; i < argc; ++i)
    {
        const std::string AdapterArgName = "--adapter=";

        const auto* arg = argv[i];
        if (strcmp(arg, "--mode=d3d11") == 0)
        {
            TestEnvCI.deviceType = RENDER_DEVICE_TYPE_D3D11;
        }
        else if (strcmp(arg, "--mode=d3d11_sw") == 0)
        {
            TestEnvCI.deviceType  = RENDER_DEVICE_TYPE_D3D11;
            TestEnvCI.AdapterType = ADAPTER_TYPE_SOFTWARE;
        }
        else if (strcmp(arg, "--mode=d3d12") == 0)
        {
            TestEnvCI.deviceType = RENDER_DEVICE_TYPE_D3D12;
        }
        else if (strcmp(arg, "--mode=d3d12_sw") == 0)
        {
            TestEnvCI.deviceType  = RENDER_DEVICE_TYPE_D3D12;
            TestEnvCI.AdapterType = ADAPTER_TYPE_SOFTWARE;
        }
        else if (strcmp(arg, "--mode=vk") == 0)
        {
            TestEnvCI.deviceType = RENDER_DEVICE_TYPE_VULKAN;
        }
        else if (strcmp(arg, "--mode=gl") == 0)
        {
            TestEnvCI.deviceType = RENDER_DEVICE_TYPE_GL;
        }
        else if (strcmp(arg, "--mode=mtl") == 0)
        {
            TestEnvCI.deviceType = RENDER_DEVICE_TYPE_METAL;
        }
        else if (AdapterArgName.compare(0, AdapterArgName.length(), arg, AdapterArgName.length()) == 0)
        {
            TestEnvCI.AdapterId = static_cast<Uint32>(atoi(arg + AdapterArgName.length()));
        }
        else if (strcmp(arg, "--shader_compiler=dxc") == 0)
        {
            ShCompiler = SHADER_COMPILER_DXC;
        }
        else if (strcmp(arg, "--non_separable_progs") == 0)
        {
            TestEnvCI.ForceNonSeparablePrograms = true;
        }
    }

    if (TestEnvCI.deviceType == RENDER_DEVICE_TYPE_UNDEFINED)
    {
        LOG_ERROR_MESSAGE("Device type is not specified");
        return -1;
    }

    if (TestEnvCI.ForceNonSeparablePrograms && TestEnvCI.deviceType != RENDER_DEVICE_TYPE_GL)
    {
        LOG_ERROR_MESSAGE("Non-separable programs can only be forced for OpenGL device.");
    }

    SwapChainDesc SCDesc;
    SCDesc.Width             = 512;
    SCDesc.Height            = 512;
    SCDesc.ColorBufferFormat = TEX_FORMAT_RGBA8_UNORM;
    SCDesc.DepthBufferFormat = TEX_FORMAT_D32_FLOAT;

    Diligent::Testing::TestingEnvironment* pEnv = nullptr;
    try
    {
        switch (TestEnvCI.deviceType)
        {
#if D3D11_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D11:
                if (TestEnvCI.AdapterType == ADAPTER_TYPE_SOFTWARE)
                    std::cout << "\n\n\n================ Testing Diligent Core API in Direct3D11-SW mode =================\n\n";
                else
                    std::cout << "\n\n\n================== Testing Diligent Core API in Direct3D11 mode ==================\n\n";
                pEnv = CreateTestingEnvironmentD3D11(TestEnvCI, SCDesc);
                break;
#endif

#if D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
                if (TestEnvCI.AdapterType == ADAPTER_TYPE_SOFTWARE)
                    std::cout << "\n\n\n================ Testing Diligent Core API in Direct3D12-SW mode =================\n\n";
                else
                    std::cout << "\n\n\n================== Testing Diligent Core API in Direct3D12 mode ==================\n\n";
                pEnv = CreateTestingEnvironmentD3D12(TestEnvCI, SCDesc);
                break;
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
            case RENDER_DEVICE_TYPE_GL:
            case RENDER_DEVICE_TYPE_GLES:
                std::cout << "\n\n\n==================== Testing Diligent Core API in OpenGL mode ====================\n\n";
                if (TestEnvCI.ForceNonSeparablePrograms)
                    std::cout << "Forcing non-separable shader programs\n";
                pEnv = CreateTestingEnvironmentGL(TestEnvCI, SCDesc);
                break;

#endif

#if VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
                std::cout << "\n\n\n==================== Testing Diligent Core API in Vulkan mode ====================\n\n";
                pEnv = CreateTestingEnvironmentVk(TestEnvCI, SCDesc);
                break;
#endif

#if METAL_SUPPORTED
            case RENDER_DEVICE_TYPE_METAL:
                std::cout << "\n\n\n==================== Testing Diligent Core API in Metal mode ====================\n\n";
                pEnv = CreateTestingEnvironmentMtl(TestEnvCI, SCDesc);
                break;
#endif

            default:
                LOG_ERROR_AND_THROW("Unsupported device type");
        }
    }
    catch (...)
    {
        return -1;
    }
    pEnv->SetDefaultCompiler(ShCompiler);
    ::testing::AddGlobalTestEnvironment(pEnv);

    auto ret_val = RUN_ALL_TESTS();
    std::cout << "\n\n\n";
    return ret_val;
}
