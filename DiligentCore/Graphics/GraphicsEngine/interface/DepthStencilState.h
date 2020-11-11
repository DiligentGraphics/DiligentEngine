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

// clang-format off

/// \file
/// Definition of data types that describe depth-stencil state

#include "GraphicsTypes.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)


/// Stencil operation

/// [D3D11_STENCIL_OP]: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476219(v=vs.85).aspx
/// [D3D12_STENCIL_OP]: https://msdn.microsoft.com/en-us/library/windows/desktop/dn770409(v=vs.85).aspx
/// This enumeration describes the stencil operation and generally mirrors
/// [D3D11_STENCIL_OP][]/[D3D12_STENCIL_OP][] enumeration. 
/// It is used by Diligent::StencilOpDesc structure to describe the stencil fail, depth fail
/// and stencil pass operations
DILIGENT_TYPED_ENUM(STENCIL_OP, Int8)
{
    /// Undefined operation.
    STENCIL_OP_UNDEFINED = 0,

    /// Keep the existing stencil data.\n
    /// Direct3D counterpart: D3D11_STENCIL_OP_KEEP/D3D12_STENCIL_OP_KEEP. OpenGL counterpart: GL_KEEP.
    STENCIL_OP_KEEP      = 1,

    /// Set the stencil data to 0.\n
    /// Direct3D counterpart: D3D11_STENCIL_OP_ZERO/D3D12_STENCIL_OP_ZERO. OpenGL counterpart: GL_ZERO.
    STENCIL_OP_ZERO      = 2,

    /// Set the stencil data to the reference value set by calling IDeviceContext::SetStencilRef().\n
    /// Direct3D counterpart: D3D11_STENCIL_OP_REPLACE/D3D12_STENCIL_OP_REPLACE. OpenGL counterpart: GL_REPLACE.
    STENCIL_OP_REPLACE   = 3,
    
    /// Increment the current stencil value, and clamp to the maximum representable unsigned value.\n
    /// Direct3D counterpart: D3D11_STENCIL_OP_INCR_SAT/D3D12_STENCIL_OP_INCR_SAT. OpenGL counterpart: GL_INCR.
    STENCIL_OP_INCR_SAT  = 4,

    /// Decrement the current stencil value, and clamp to 0.\n
    /// Direct3D counterpart: D3D11_STENCIL_OP_DECR_SAT/D3D12_STENCIL_OP_DECR_SAT. OpenGL counterpart: GL_DECR.
    STENCIL_OP_DECR_SAT  = 5,

    /// Bitwise invert the current stencil buffer value.\n
    /// Direct3D counterpart: D3D11_STENCIL_OP_INVERT/D3D12_STENCIL_OP_INVERT. OpenGL counterpart: GL_INVERT.
    STENCIL_OP_INVERT    = 6,

    /// Increment the current stencil value, and wrap the value to zero when incrementing 
    /// the maximum representable unsigned value. \n
    /// Direct3D counterpart: D3D11_STENCIL_OP_INCR/D3D12_STENCIL_OP_INCR. OpenGL counterpart: GL_INCR_WRAP.
    STENCIL_OP_INCR_WRAP = 7,

    /// Decrement the current stencil value, and wrap the value to the maximum representable 
    /// unsigned value when decrementing a value of zero.\n
    /// Direct3D counterpart: D3D11_STENCIL_OP_DECR/D3D12_STENCIL_OP_DECR. OpenGL counterpart: GL_DECR_WRAP.
    STENCIL_OP_DECR_WRAP = 8,

    /// Helper value that stores the total number of stencil operations in the enumeration.
    STENCIL_OP_NUM_OPS
};

/// Describes stencil operations that are performed based on the results of depth test.

/// [D3D11_DEPTH_STENCILOP_DESC]: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476109(v=vs.85).aspx
/// [D3D12_DEPTH_STENCILOP_DESC]: https://msdn.microsoft.com/en-us/library/windows/desktop/dn770355(v=vs.85).aspx
/// The structure generally mirrors [D3D11_DEPTH_STENCILOP_DESC][]/[D3D12_DEPTH_STENCILOP_DESC][] structure. 
/// It is used by Diligent::DepthStencilStateDesc structure to describe the stencil 
/// operations for the front and back facing polygons.
struct StencilOpDesc
{
    /// The stencil operation to perform when stencil testing fails.
    /// Default value: Diligent::STENCIL_OP_KEEP.
    STENCIL_OP          StencilFailOp       DEFAULT_INITIALIZER(STENCIL_OP_KEEP);

    /// The stencil operation to perform when stencil testing passes and depth testing fails.
    /// Default value: Diligent::STENCIL_OP_KEEP.
    STENCIL_OP          StencilDepthFailOp  DEFAULT_INITIALIZER(STENCIL_OP_KEEP);

    /// The stencil operation to perform when stencil testing and depth testing both pass.
    /// Default value: Diligent::STENCIL_OP_KEEP.
    STENCIL_OP          StencilPassOp       DEFAULT_INITIALIZER(STENCIL_OP_KEEP);

    /// A function that compares stencil data against existing stencil data. 
    /// Default value: Diligent::COMPARISON_FUNC_ALWAYS. See Diligent::COMPARISON_FUNCTION.
    COMPARISON_FUNCTION StencilFunc         DEFAULT_INITIALIZER(COMPARISON_FUNC_ALWAYS);


#if DILIGENT_CPP_INTERFACE
    // We have to explicitly define constructors because otherwise Apple's clang fails to compile the following legitimate code:
    //     StencilOpDesc{STENCIL_OP_KEEP, STENCIL_OP_KEEP, STENCIL_OP_KEEP, COMPARISON_FUNC_ALWAYS}

    StencilOpDesc()noexcept{}

    StencilOpDesc(STENCIL_OP          _StencilFailOp,
                  STENCIL_OP          _StencilDepthFailOp,
                  STENCIL_OP          _StencilPassOp,
                  COMPARISON_FUNCTION _StencilFunc)noexcept : 
        StencilFailOp      {_StencilFailOp     },
        StencilDepthFailOp {_StencilDepthFailOp},
        StencilPassOp      {_StencilPassOp     },
        StencilFunc        {_StencilFunc       }
    {}

    /// Tests if two structures are equivalent

    /// \param [in] rhs - reference to the structure to perform comparison with
    /// \return 
    /// - True if all members of the two structures are equal.
    /// - False otherwise
    bool operator== (const StencilOpDesc& rhs) const
    {
        return StencilFailOp      == rhs.StencilFailOp      &&
               StencilDepthFailOp == rhs.StencilDepthFailOp &&
               StencilPassOp      == rhs.StencilPassOp      &&
               StencilFunc        == rhs.StencilFunc;
    }
#endif
};
typedef struct StencilOpDesc StencilOpDesc;

/// Depth stencil state description

/// [D3D11_DEPTH_STENCIL_DESC]: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476110(v=vs.85).aspx
/// [D3D12_DEPTH_STENCIL_DESC]: https://msdn.microsoft.com/en-us/library/windows/desktop/dn770356(v=vs.85).aspx
/// This structure describes the depth stencil state and is part of the GraphicsPipelineDesc.
/// The structure generally mirrors [D3D11_DEPTH_STENCIL_DESC][]/[D3D12_DEPTH_STENCIL_DESC][]
/// structure.
struct DepthStencilStateDesc
{
    /// Enable depth-stencil operations. When it is set to False, 
    /// depth test always passes, depth writes are disabled,
    /// and no stencil operations are performed. Default value: True.
    Bool                DepthEnable         DEFAULT_INITIALIZER(True);

    /// Enable or disable writes to a depth buffer. Default value: True.
    Bool                DepthWriteEnable    DEFAULT_INITIALIZER(True);

    /// A function that compares depth data against existing depth data. 
    /// See Diligent::COMPARISON_FUNCTION for details.
    /// Default value: Diligent::COMPARISON_FUNC_LESS.
    COMPARISON_FUNCTION DepthFunc           DEFAULT_INITIALIZER(COMPARISON_FUNC_LESS);

    /// Enable stencil opertaions. Default value: False.
    Bool                StencilEnable       DEFAULT_INITIALIZER(False);
    
    /// Identify which bits of the depth-stencil buffer are accessed when reading stencil data.
    /// Default value: 0xFF.
    Uint8               StencilReadMask     DEFAULT_INITIALIZER(0xFF);
    
    /// Identify which bits of the depth-stencil buffer are accessed when writing stencil data.
    /// Default value: 0xFF.
    Uint8               StencilWriteMask    DEFAULT_INITIALIZER(0xFF);

    /// Identify stencil operations for the front-facing triangles, see Diligent::StencilOpDesc.
    StencilOpDesc FrontFace;

    /// Identify stencil operations for the back-facing triangles, see Diligent::StencilOpDesc.
    StencilOpDesc BackFace;


#if DILIGENT_CPP_INTERFACE
    // We have to explicitly define constructors because otherwise Apple's clang fails to compile the following legitimate code:
    //     DepthStencilStateDesc{False, False}

    DepthStencilStateDesc()noexcept{}

    DepthStencilStateDesc(Bool                _DepthEnable,
                          Bool                _DepthWriteEnable,
                          COMPARISON_FUNCTION _DepthFunc        = DepthStencilStateDesc{}.DepthFunc,
                          Bool                _StencilEnable    = DepthStencilStateDesc{}.StencilEnable,
                          Uint8               _StencilReadMask  = DepthStencilStateDesc{}.StencilReadMask,
                          Uint8               _StencilWriteMask = DepthStencilStateDesc{}.StencilWriteMask,
                          StencilOpDesc       _FrontFace        = StencilOpDesc{},
                          StencilOpDesc       _BackFace         = StencilOpDesc{})noexcept : 
        DepthEnable     {_DepthEnable     },
        DepthWriteEnable{_DepthWriteEnable},
        DepthFunc       {_DepthFunc       },
        StencilEnable   {_StencilEnable   },
        StencilReadMask {_StencilReadMask },
        StencilWriteMask{_StencilWriteMask},
        FrontFace       {_FrontFace       },
        BackFace        {_BackFace        }
    {}

    /// Tests if two structures are equivalent

    /// \param [in] rhs - reference to the structure to perform comparison with
    /// \return 
    /// - True if all members of the two structures are equal.
    /// - False otherwise
    bool operator== (const DepthStencilStateDesc& rhs) const
    {
        return  DepthEnable      == rhs.DepthEnable      &&
                DepthWriteEnable == rhs.DepthWriteEnable &&
                DepthFunc        == rhs.DepthFunc        &&
                StencilEnable    == rhs.StencilEnable    &&
                StencilReadMask  == rhs.StencilReadMask  &&
                StencilWriteMask == rhs.StencilWriteMask &&
                FrontFace        == rhs.FrontFace        &&
                BackFace         == rhs.BackFace;     
    }
#endif
};
typedef struct DepthStencilStateDesc DepthStencilStateDesc;

DILIGENT_END_NAMESPACE
