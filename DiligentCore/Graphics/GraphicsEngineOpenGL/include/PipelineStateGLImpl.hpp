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

#include <vector>
#include "PipelineStateGL.h"
#include "PipelineStateBase.hpp"
#include "RenderDevice.h"
#include "GLObjectWrapper.hpp"
#include "GLContext.hpp"
#include "RenderDeviceGLImpl.hpp"
#include "GLProgramResources.hpp"
#include "GLPipelineResourceLayout.hpp"
#include "GLProgramResourceCache.hpp"
#include "ShaderGLImpl.hpp"

namespace Diligent
{

class FixedBlockMemoryAllocator;

/// Pipeline state object implementation in OpenGL backend.
class PipelineStateGLImpl final : public PipelineStateBase<IPipelineStateGL, RenderDeviceGLImpl>
{
public:
    using TPipelineStateBase = PipelineStateBase<IPipelineStateGL, RenderDeviceGLImpl>;

    PipelineStateGLImpl(IReferenceCounters*                    pRefCounters,
                        RenderDeviceGLImpl*                    pDeviceGL,
                        const GraphicsPipelineStateCreateInfo& CreateInfo,
                        bool                                   IsDeviceInternal = false);
    PipelineStateGLImpl(IReferenceCounters*                   pRefCounters,
                        RenderDeviceGLImpl*                   pDeviceGL,
                        const ComputePipelineStateCreateInfo& CreateInfo,
                        bool                                  IsDeviceInternal = false);
    ~PipelineStateGLImpl();

    /// Queries the specific interface, see IObject::QueryInterface() for details
    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override;

    /// Implementation of IPipelineState::BindStaticResources() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE BindStaticResources(Uint32            ShaderFlags,
                                                        IResourceMapping* pResourceMapping,
                                                        Uint32            Flags) override final;

    /// Implementation of IPipelineState::GetStaticVariableCount() in OpenGL backend.
    virtual Uint32 DILIGENT_CALL_TYPE GetStaticVariableCount(SHADER_TYPE ShaderType) const override final;

    /// Implementation of IPipelineState::GetStaticVariableByName() in OpenGL backend.
    virtual IShaderResourceVariable* DILIGENT_CALL_TYPE GetStaticVariableByName(SHADER_TYPE ShaderType, const Char* Name) override final;

    /// Implementation of IPipelineState::GetStaticVariableByIndex() in OpenGL backend.
    virtual IShaderResourceVariable* DILIGENT_CALL_TYPE GetStaticVariableByIndex(SHADER_TYPE ShaderType, Uint32 Index) override final;

    /// Implementation of IPipelineState::CreateShaderResourceBinding() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE CreateShaderResourceBinding(IShaderResourceBinding** ppShaderResourceBinding,
                                                                bool                     InitStaticResources) override final;

    /// Implementation of IPipelineState::IsCompatibleWith() in OpenGL backend.
    virtual bool DILIGENT_CALL_TYPE IsCompatibleWith(const IPipelineState* pPSO) const override final;

    void CommitProgram(GLContextState& State);

    void InitializeSRBResourceCache(GLProgramResourceCache& ResourceCache) const;

    const GLPipelineResourceLayout& GetResourceLayout() const { return m_ResourceLayout; }
    const GLPipelineResourceLayout& GetStaticResourceLayout() const { return m_StaticResourceLayout; }
    const GLProgramResourceCache&   GetStaticResourceCache() const { return m_StaticResourceCache; }

private:
    GLObjectWrappers::GLPipelineObj& GetGLProgramPipeline(GLContext::NativeGLContextType Context);

    void InitImmutableSamplersInResourceCache(const GLPipelineResourceLayout& ResourceLayout, GLProgramResourceCache& Cache) const;

    struct GLPipelineShaderStageInfo
    {
        const SHADER_TYPE   Type;
        ShaderGLImpl* const pShader;
        GLPipelineShaderStageInfo(SHADER_TYPE   _Type,
                                  ShaderGLImpl* _pShader) :
            Type{_Type},
            pShader{_pShader}
        {}
    };

    template <typename PSOCreateInfoType>
    void Initialize(const PSOCreateInfoType& CreateInfo, const std::vector<GLPipelineShaderStageInfo>& ShaderStages);

    void InitResourceLayouts(const std::vector<GLPipelineShaderStageInfo>& ShaderStages,
                             LinearAllocator&                              MemPool);

    void Destruct();

    // Linked GL programs for every shader stage. Every pipeline needs to have its own programs
    // because resource bindings assigned by GLProgramResources::LoadUniforms depend on other
    // shader stages.
    using GLProgramObj         = GLObjectWrappers::GLProgramObj;
    GLProgramObj* m_GLPrograms = nullptr; // [m_NumShaderStages]

    ThreadingTools::LockFlag m_ProgPipelineLockFlag;

    std::vector<std::pair<GLContext::NativeGLContextType, GLObjectWrappers::GLPipelineObj>> m_GLProgPipelines;

    // Resource layout that keeps variables of all types, but does not reference a
    // resource cache.
    // This layout is used by SRB objects to initialize only mutable and dynamic variables and by
    // DeviceContextGLImpl::BindProgramResources to verify resource bindings.
    GLPipelineResourceLayout m_ResourceLayout;

    // Resource layout that only keeps static variables
    GLPipelineResourceLayout m_StaticResourceLayout;
    // Resource cache for static resource variables only
    GLProgramResourceCache m_StaticResourceCache;

    // Program resources for all shader stages in the pipeline
    GLProgramResources* m_ProgramResources = nullptr; // [m_NumShaderStages]

    Uint32 m_TotalUniformBufferBindings = 0;
    Uint32 m_TotalSamplerBindings       = 0;
    Uint32 m_TotalImageBindings         = 0;
    Uint32 m_TotalStorageBufferBindings = 0;

    using SamplerPtr                = RefCntAutoPtr<ISampler>;
    SamplerPtr* m_ImmutableSamplers = nullptr; // [m_Desc.ResourceLayout.NumImmutableSamplers]
};

} // namespace Diligent
