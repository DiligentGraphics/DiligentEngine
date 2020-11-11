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

#include "BaseInterfacesGL.h"
#include "ShaderGL.h"
#include "ShaderBase.hpp"
#include "RenderDevice.h"
#include "GLObjectWrapper.hpp"
#include "RenderDeviceGLImpl.hpp"
#include "GLProgramResources.hpp"

namespace Diligent
{

class FixedBlockMemoryAllocator;

inline GLenum GetGLShaderType(SHADER_TYPE ShaderType)
{
    switch (ShaderType)
    {
        // clang-format off
        case SHADER_TYPE_VERTEX:    return GL_VERTEX_SHADER;          break;
        case SHADER_TYPE_PIXEL:     return GL_FRAGMENT_SHADER;        break;
        case SHADER_TYPE_GEOMETRY:  return GL_GEOMETRY_SHADER;        break;
        case SHADER_TYPE_HULL:      return GL_TESS_CONTROL_SHADER;    break;
        case SHADER_TYPE_DOMAIN:    return GL_TESS_EVALUATION_SHADER; break;
        case SHADER_TYPE_COMPUTE:   return GL_COMPUTE_SHADER;         break;
        default: return 0;
            // clang-format on
    }
}

inline GLenum ShaderTypeToGLShaderBit(SHADER_TYPE ShaderType)
{
    switch (ShaderType)
    {
        // clang-format off
        case SHADER_TYPE_VERTEX:    return GL_VERTEX_SHADER_BIT;          break;
        case SHADER_TYPE_PIXEL:     return GL_FRAGMENT_SHADER_BIT;        break;
        case SHADER_TYPE_GEOMETRY:  return GL_GEOMETRY_SHADER_BIT;        break;
        case SHADER_TYPE_HULL:      return GL_TESS_CONTROL_SHADER_BIT;    break;
        case SHADER_TYPE_DOMAIN:    return GL_TESS_EVALUATION_SHADER_BIT; break;
        case SHADER_TYPE_COMPUTE:   return GL_COMPUTE_SHADER_BIT;         break;
        default: return 0;
            // clang-format on
    }
}

/// Shader object implementation in OpenGL backend.
class ShaderGLImpl final : public ShaderBase<IShaderGL, RenderDeviceGLImpl>
{
public:
    using TShaderBase = ShaderBase<IShaderGL, RenderDeviceGLImpl>;

    ShaderGLImpl(IReferenceCounters*     pRefCounters,
                 RenderDeviceGLImpl*     pDeviceGL,
                 const ShaderCreateInfo& ShaderCI,
                 bool                    bIsDeviceInternal = false);
    ~ShaderGLImpl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of IShader::GetResourceCount() in OpenGL backend.
    virtual Uint32 DILIGENT_CALL_TYPE GetResourceCount() const override final;

    /// Implementation of IShader::GetResource() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE GetResourceDesc(Uint32 Index, ShaderResourceDesc& ResourceDesc) const override final;

    static GLObjectWrappers::GLProgramObj LinkProgram(ShaderGLImpl** ppShaders, Uint32 NumShaders, bool IsSeparableProgram);

private:
    GLObjectWrappers::GLShaderObj m_GLShaderObj;
    GLProgramResources            m_Resources;
};

} // namespace Diligent
