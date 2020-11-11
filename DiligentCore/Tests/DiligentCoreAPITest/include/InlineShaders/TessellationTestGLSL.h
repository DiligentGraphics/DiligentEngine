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

#include <string>

namespace
{

namespace GLSL
{

// clang-format off
const std::string TessTest_VS{
R"(
#version 420 core

#ifndef GL_ES
out gl_PerVertex
{
    vec4 gl_Position;
};
#endif

layout(location = 0) out vec3 out_Color;

void main()
{
    vec4 Positions[2];
    Positions[0] = vec4(-0.5, -0.5, 0.0, 1.0);
    Positions[1] = vec4(+0.5, +0.5, 0.0, 1.0);

    vec3 Color[2];
    Color[0] = vec3(0.5, 0.0, 0.0);
    Color[1] = vec3(0.0, 0.0, 0.5);

#ifdef VULKAN
    gl_Position = Positions[gl_VertexIndex];
    out_Color   = Color    [gl_VertexIndex];
#else
    gl_Position = Positions[gl_VertexID];
    out_Color   = Color    [gl_VertexID];
#endif
}
)"
};

const std::string TessTest_TCS{
R"(
#version 420 core
#extension GL_EXT_tessellation_shader : enable

in gl_PerVertex
{
    vec4 gl_Position;
    //float gl_PointSize;
    //float gl_ClipDistance[];
} gl_in[gl_MaxPatchVertices];

out gl_PerVertex
{
    vec4 gl_Position;
    //float gl_PointSize;
    //float gl_ClipDistance[];
} gl_out[];

layout(vertices = 1) out;
layout(location = 0) in  vec3 in_Color[];
layout(location = 0) out vec3 out_Color[];

void main()
{
    gl_TessLevelOuter[0] = 2.5;
    gl_TessLevelOuter[1] = 4.25;
    gl_TessLevelOuter[2] = 5.75;
    gl_TessLevelOuter[3] = 7.5;

    gl_TessLevelInner[0] = 6.75;
    gl_TessLevelInner[1] = 7.25;

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    out_Color[gl_InvocationID] = in_Color[gl_InvocationID];
}
)"
};


const std::string TessTest_TES{
R"(
#version 420 core
#extension GL_EXT_tessellation_shader : enable

in gl_PerVertex
{
    vec4 gl_Position;
    //float gl_PointSize;
    //float gl_ClipDistance[];
} gl_in[gl_MaxPatchVertices];

out gl_PerVertex
{
    vec4 gl_Position;
    //float gl_PointSize;
    //float gl_ClipDistance[];
};

layout(quads, fractional_even_spacing, ccw)in;
layout(location = 0) in  vec3 in_Color[];
layout(location = 0) out vec3 out_Color;

void main()
{
    vec2 QuadUV = gl_TessCoord.xy;
    gl_Position.xy = gl_in[0].gl_Position.xy + (QuadUV.xy - vec2(0.5, 0.5)) * 0.875;
    gl_Position.zw = gl_in[0].gl_Position.zw;
    out_Color = in_Color[0] + vec3(QuadUV.xy, 1.0 - QuadUV.x * QuadUV.y) * 0.75;
}
)"
};

const std::string TessTest_FS{
R"(
#version 420 core

layout(location = 0) in  vec3 in_Color;
layout(location = 0) out vec4 out_Color;

void main()
{
    out_Color = vec4(in_Color, 1.0);
}
)"
};
// clang-format on

} // namespace GLSL

} // namespace
