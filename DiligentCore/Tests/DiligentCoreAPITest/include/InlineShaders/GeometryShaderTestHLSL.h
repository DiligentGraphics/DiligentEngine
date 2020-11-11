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
const std::string GSTest_VS{
R"(
struct GSInput 
{ 
    float4 Pos : SV_POSITION; 
};

void main(in  uint    VertId : SV_VertexID,
          out GSInput GSIn) 
{
    float4 Pos[2];
    Pos[0] = float4(-0.5, -0.25, 0.0, 1.0);
    Pos[1] = float4(+0.5, +0.25, 0.0, 1.0);

    GSIn.Pos = Pos[VertId];
}
)"
};

const std::string GSTest_GS{
R"(
struct GSInput 
{ 
    float4 Pos : SV_POSITION; 
};

struct PSInput 
{ 
    float4 Pos   : SV_POSITION; 
    float3 Color : COLOR; 
};

[maxvertexcount(3)]
void main(point GSInput In[1], 
          inout TriangleStream<PSInput> triStream )
{
    float4 Pos[3];
    Pos[0] = float4(-1.0, -1.0, 0.0, 1.0);
    Pos[1] = float4( 0.0, +1.0, 0.0, 1.0);
    Pos[2] = float4(+1.0, -1.0, 0.0, 1.0);

    float3 Col[3];
    Col[0] = float3(1.0, 0.0, 0.0);
    Col[1] = float3(0.0, 1.0, 0.0);
    Col[2] = float3(0.0, 0.0, 1.0);

    for (int i=0; i<3; ++i)
    {
        PSInput PSIn;
        PSIn.Pos = float4(Pos[i].xy * 0.5 + In[0].Pos.xy, Pos[i].zw);
        PSIn.Color = Col[i];
        triStream.Append(PSIn);
    }
}
)"
};

const std::string GSTest_PS{
R"(
struct PSInput 
{ 
    float4 Pos   : SV_POSITION; 
    float3 Color : COLOR; 
};

struct PSOutput
{ 
    float4 Color : SV_TARGET; 
};

void main(in  PSInput  PSIn,
          out PSOutput PSOut)
{
    PSOut.Color = float4(PSIn.Color.rgb, 1.0);
}
)"
};
// clang-format on

} // namespace HLSL

} // namespace
