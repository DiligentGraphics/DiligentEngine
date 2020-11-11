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
const std::string MeshShaderTest_MS{
R"(
#version 460
#extension GL_NV_mesh_shader : require

layout(local_size_x=4) in;
layout(max_vertices=4, max_primitives=2) out;
layout(triangles) out;

//out uint gl_PrimitiveCountNV;
//out uint gl_PrimitiveIndicesNV[max_primitives * 3]

//out gl_MeshPerVertexNV {
//  vec4 gl_Position;
//} gl_MeshVerticesNV[max_vertices]

layout(location = 0) out vec3 out_Color[];

const vec3 colors[4] = {vec3(1.0,0.0,0.0), vec3(0.0,1.0,0.0), vec3(0.0,0.0,1.0), vec3(1.0,1.0,1.0)};

void main ()
{
    const uint I = gl_LocalInvocationID.x;

    // only one thread writes output primitve count
    if (I == 0)
    {
        gl_PrimitiveCountNV = 2;
    }

    // first triangle
    if (I == 0)
    {
        gl_PrimitiveIndicesNV[0] = 0;
        gl_PrimitiveIndicesNV[1] = 1;
        gl_PrimitiveIndicesNV[2] = 2;
    }

    // second triangle
    if (I == 3)
    {
        gl_PrimitiveIndicesNV[3] = 2;
        gl_PrimitiveIndicesNV[4] = 1;
        gl_PrimitiveIndicesNV[5] = 3;
    }

    gl_MeshVerticesNV[I].gl_Position = vec4(float(I >> 1) * 2.0 - 1.0, float(I & 1) * 2.0 - 1.0, 0.0, 1.0);
 
    out_Color[I] = colors[I];
}
)"
};

const std::string MeshShaderTest_FS{
R"(
#version 460
 
layout(location = 0) in  vec3 in_Color;
layout(location = 0) out vec4 out_Color;
 
void main()
{
    out_Color = vec4(in_Color, 1.0);
}
)"
};


const std::string AmplificationShaderTest_TS{
R"(
#version 460
#extension GL_NV_mesh_shader : require

layout(local_size_x = 8) in;

taskNV out Task {
    uint baseID;
    uint subIDs[8];
} Output;

void main()
{
    const uint I = gl_LocalInvocationID.x;

    if (I == 0)
    {
        gl_TaskCountNV = 8;
        Output.baseID = gl_WorkGroupID.x * 8;
    }

    Output.subIDs[I] = I;
}
)"
};

const std::string AmplificationShaderTest_MS{
R"(
#version 460
#extension GL_NV_mesh_shader : require

layout(local_size_x = 1) in;
layout(max_vertices = 3, max_primitives = 1) out;
layout(triangles) out;

taskNV in Task {
    uint baseID;
    uint subIDs[8];
} Input;

layout(location = 0) out vec3 out_Color[];

const vec3 colors[4] = {vec3(1.0,0.0,0.0), vec3(0.0,1.0,0.0), vec3(0.0,0.0,1.0), vec3(1.0,0.0,1.0)};

void main ()
{
    uint meshletID = Input.baseID + Input.subIDs[gl_WorkGroupID.x];

    vec2 center;
    center.x = (float((meshletID % 9) + 1) / 10.0) * 2.0 - 1.0;
    center.y = (float((meshletID / 9) + 1) / 10.0) * 2.0 - 1.0;

    gl_PrimitiveCountNV = 1;

    gl_PrimitiveIndicesNV[0] = 2;
    gl_PrimitiveIndicesNV[1] = 1;
    gl_PrimitiveIndicesNV[2] = 0;
   
    gl_MeshVerticesNV[0].gl_Position = vec4(center.x, center.y + 0.09, 0.0, 1.0);
    gl_MeshVerticesNV[1].gl_Position = vec4(center.x - 0.09, center.y - 0.09, 0.0, 1.0);
    gl_MeshVerticesNV[2].gl_Position = vec4(center.x + 0.09, center.y - 0.09, 0.0, 1.0);
 
    out_Color[0] = colors[meshletID & 3];
    out_Color[1] = colors[meshletID & 3];
    out_Color[2] = colors[meshletID & 3];
}
)"
};

const std::string AmplificationShaderTest_FS{
R"(
#version 450
 
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
