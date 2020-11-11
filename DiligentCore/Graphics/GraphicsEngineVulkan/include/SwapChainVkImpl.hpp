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

/// \file
/// Declaration of Diligent::SwapChainVkImpl class

#include "SwapChainVk.h"
#include "SwapChainBase.hpp"
#include "VulkanUtilities/VulkanInstance.hpp"
#include "VulkanUtilities/VulkanObjectWrappers.hpp"
#include "ManagedVulkanObject.hpp"

namespace Diligent
{

/// Swap chain implementation in Vulkan backend.
class SwapChainVkImpl final : public SwapChainBase<ISwapChainVk>
{
public:
    using TSwapChainBase = SwapChainBase<ISwapChainVk>;

    SwapChainVkImpl(IReferenceCounters*        pRefCounters,
                    const SwapChainDesc&       SwapChainDesc,
                    class RenderDeviceVkImpl*  pRenderDeviceVk,
                    class DeviceContextVkImpl* pDeviceContextVk,
                    const NativeWindow&        Window);
    ~SwapChainVkImpl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of ISwapChain::Present() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE Present(Uint32 SyncInterval) override final;

    /// Implementation of ISwapChain::Resize() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE Resize(Uint32 NewWidth, Uint32 NewHeight, SURFACE_TRANSFORM NewPreTransform) override final;

    /// Implementation of ISwapChain::SetFullscreenMode() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE SetFullscreenMode(const DisplayModeAttribs& DisplayMode) override final;

    /// Implementation of ISwapChain::SetWindowedMode() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE SetWindowedMode() override final;

    /// Implementation of ISwapChainVk::GetVkSwapChain().
    virtual VkSwapchainKHR DILIGENT_CALL_TYPE GetVkSwapChain() override final { return m_VkSwapChain; }

    /// Implementation of ISwapChain::GetCurrentBackBufferRTV() in Vulkan backend.
    virtual ITextureViewVk* DILIGENT_CALL_TYPE GetCurrentBackBufferRTV() override final
    {
        VERIFY_EXPR(m_BackBufferIndex >= 0 && m_BackBufferIndex < m_SwapChainDesc.BufferCount);
        return m_pBackBufferRTV[m_BackBufferIndex];
    }

    /// Implementation of ISwapChain::GetDepthBufferDSV() in Vulkan backend.
    virtual ITextureViewVk* DILIGENT_CALL_TYPE GetDepthBufferDSV() override final { return m_pDepthBufferDSV; }

private:
    void     CreateSurface();
    void     CreateVulkanSwapChain();
    void     InitBuffersAndViews();
    VkResult AcquireNextImage(DeviceContextVkImpl* pDeviceCtxVk);
    void     RecreateVulkanSwapchain(DeviceContextVkImpl* pImmediateCtxVk);
    void     WaitForImageAcquiredFences();
    void     ReleaseSwapChainResources(DeviceContextVkImpl* pImmediateCtxVk, bool DestroyVkSwapChain);

    const NativeWindow m_Window;

    std::shared_ptr<const VulkanUtilities::VulkanInstance> m_VulkanInstance;

    Uint32 m_DesiredBufferCount = 0;

    VkSurfaceKHR   m_VkSurface     = VK_NULL_HANDLE;
    VkSwapchainKHR m_VkSwapChain   = VK_NULL_HANDLE;
    VkFormat       m_VkColorFormat = VK_FORMAT_UNDEFINED;

#if PLATFORM_ANDROID
    // Surface extent corresponding to identity transform. We have to store this value,
    // because on Android vkGetPhysicalDeviceSurfaceCapabilitiesKHR is not reliable and
    // starts reporting incorrect dimensions after few rotations.
    VkExtent2D m_SurfaceIdentityExtent = {};

    // Keep track of current surface transform to detect orientation changes.
    VkSurfaceTransformFlagBitsKHR m_CurrentSurfaceTransform = VK_SURFACE_TRANSFORM_FLAG_BITS_MAX_ENUM_KHR;
#endif

    std::vector<RefCntAutoPtr<ManagedSemaphore>> m_ImageAcquiredSemaphores;
    std::vector<RefCntAutoPtr<ManagedSemaphore>> m_DrawCompleteSemaphores;
    std::vector<VulkanUtilities::FenceWrapper>   m_ImageAcquiredFences;

    std::vector<RefCntAutoPtr<ITextureViewVk>, STDAllocatorRawMem<RefCntAutoPtr<ITextureViewVk>>> m_pBackBufferRTV;

    std::vector<bool, STDAllocatorRawMem<bool>> m_SwapChainImagesInitialized;
    std::vector<bool, STDAllocatorRawMem<bool>> m_ImageAcquiredFenceSubmitted;

    RefCntAutoPtr<ITextureViewVk> m_pDepthBufferDSV;

    Uint32   m_SemaphoreIndex  = 0;
    uint32_t m_BackBufferIndex = 0;
    bool     m_IsMinimized     = false;
    bool     m_VSyncEnabled    = true;
};

} // namespace Diligent
