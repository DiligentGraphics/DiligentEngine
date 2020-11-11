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

namespace HLSL
{

// clang-format off
const std::string MeshShaderTest_MS{
R"(
struct VertexOut
{
    float4 Position : SV_Position;
    float3 Color    : COLOR0;
};

static const float3 colors[4] = {float3(1.0,0.0,0.0), float3(0.0,1.0,0.0), float3(0.0,0.0,1.0), float3(1.0,1.0,1.0)};

[numthreads(4,1,1)]
[outputtopology("triangle")]
void main(uint I : SV_GroupIndex,
          out indices  uint3     tris[2],
          out vertices VertexOut verts[4])
{
    // only the input values from the the first active thread are used
    SetMeshOutputCounts(4, 2);

    // first triangle
    if (I == 0)
    {
        tris[0] = uint3(0, 1, 2);
    }

    // second triangle
    if (I == 3)
    {
        tris[1] = uint3(2, 1, 3);
    }

    verts[I].Position = float4(float(I >> 1) * 2.0 - 1.0, float(I & 1) * 2.0 - 1.0, 0.0, 1.0);
    verts[I].Color = colors[I];
}
)"
};

const std::string MeshShaderTest_PS{
R"(
struct VertexOut
{
    float4 Position : SV_Position;
    float3 Color    : COLOR0;
};
 
void main(VertexOut In,
          out float4 Color : SV_Target)
{
    Color = float4(In.Color, 1.0);
}
)"
};


const std::string AmplificationShaderTest_AS{
R"(
struct Payload
{
    uint baseID;
    uint subIDs[8];
};

groupshared Payload s_payload;

[numthreads(8,1,1)]
void main(in uint I : SV_GroupIndex,
          in uint gid : SV_GroupID)
{
    if (I == 0)
        s_payload.baseID = gid * 8;

    s_payload.subIDs[I] = I;

    DispatchMesh(8, 1, 1, s_payload);
}
)"
};

const std::string AmplificationShaderTest_MS{
R"(
struct VertexOut
{
    float4 Position : SV_Position;
    float3 Color    : COLOR0;
};

struct Payload
{
    uint baseID;
    uint subIDs[8];
};

static const float3 colors[4] = {float3(1.0,0.0,0.0), float3(0.0,1.0,0.0), float3(0.0,0.0,1.0), float3(1.0,0.0,1.0)};

[numthreads(1,1,1)]
[outputtopology("triangle")]
void main(in uint gid : SV_GroupID,
          in  payload  Payload   payload,
          out indices  uint3     tris[1],
          out vertices VertexOut verts[3])
{
    // only the input values from the the first active thread are used
    SetMeshOutputCounts(3, 1);

    uint meshletID = payload.baseID + payload.subIDs[gid];

    float2 center;
    center.x = (float((meshletID % 9) + 1) / 10.0) * 2.0 - 1.0;
    center.y = (float((meshletID / 9) + 1) / 10.0) * 2.0 - 1.0;

    tris[0] = uint3(2, 1, 0);
   
    verts[0].Position = float4(center.x, center.y + 0.09, 0.0, 1.0);
    verts[1].Position = float4(center.x - 0.09, center.y - 0.09, 0.0, 1.0);
    verts[2].Position = float4(center.x + 0.09, center.y - 0.09, 0.0, 1.0);
 
    verts[0].Color = colors[meshletID & 3];
    verts[1].Color = colors[meshletID & 3];
    verts[2].Color = colors[meshletID & 3];
}
)"
};

const std::string AmplificationShaderTest_PS{
R"(
struct VertexOut
{
    float4 Position : SV_Position;
    float3 Color    : COLOR0;
};
 
void main(VertexOut In,
          out float4 Color : SV_Target)
{
    Color = float4(In.Color, 1.0);
}
)"
};
// clang-format on

} // namespace HLSL

} // namespace
