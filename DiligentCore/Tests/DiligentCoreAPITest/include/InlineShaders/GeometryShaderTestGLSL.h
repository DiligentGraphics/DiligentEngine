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
const std::string GSTest_VS{
R"(
#version 420 core

#ifndef GL_ES
out gl_PerVertex
{
	vec4 gl_Position;
};
#endif

void main()
{
    vec4 Pos[2];
    Pos[0] = vec4(-0.5, -0.25, 0.0, 1.0);
    Pos[1] = vec4(+0.5, +0.25, 0.0, 1.0);
    
#ifdef VULKAN
    gl_Position = Pos[gl_VertexIndex];
#else
    gl_Position = Pos[gl_VertexID];
#endif
}
)"
};


const std::string GSTest_GS{
R"(
#version 420 core

layout (points) in;
layout (triangle_strip, max_vertices = 3) out;

in gl_PerVertex
{
    vec4 gl_Position;
} gl_in[];

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location = 0) out vec3 out_Color;

void main() {
    vec4 Pos[3];
    Pos[0] = vec4(-1.0, -1.0, 0.0, 1.0);
    Pos[1] = vec4( 0.0, +1.0, 0.0, 1.0);
    Pos[2] = vec4(+1.0, -1.0, 0.0, 1.0);

    vec3 Col[3];
    Col[0] = vec3(1.0, 0.0, 0.0);
    Col[1] = vec3(0.0, 1.0, 0.0);
    Col[2] = vec3(0.0, 0.0, 1.0);

    for (int i=0; i < 3; ++i)
    {
        gl_Position = vec4(Pos[i].xy * 0.5 + gl_in[0].gl_Position.xy, gl_in[0].gl_Position.zw);
        out_Color = Col[i];
        EmitVertex();
    }
    EndPrimitive();   
}
)"
};

const std::string GSTest_FS{
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
