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
const std::string DrawTest_ProceduralTriangleVS{
R"(
struct PSInput 
{ 
    float4 Pos   : SV_POSITION; 
    float3 Color : COLOR; 
};

void main(in  uint    VertId : SV_VertexID,
          out PSInput PSIn) 
{
    float4 Pos[6];
    Pos[0] = float4(-1.0, -0.5, 0.0, 1.0);
    Pos[1] = float4(-0.5, +0.5, 0.0, 1.0);
    Pos[2] = float4( 0.0, -0.5, 0.0, 1.0);

    Pos[3] = float4(+0.0, -0.5, 0.0, 1.0);
    Pos[4] = float4(+0.5, +0.5, 0.0, 1.0);
    Pos[5] = float4(+1.0, -0.5, 0.0, 1.0);

    float3 Col[6];
    Col[0] = float3(1.0, 0.0, 0.0);
    Col[1] = float3(0.0, 1.0, 0.0);
    Col[2] = float3(0.0, 0.0, 1.0);

    Col[3] = float3(1.0, 0.0, 0.0);
    Col[4] = float3(0.0, 1.0, 0.0);
    Col[5] = float3(0.0, 0.0, 1.0);

    PSIn.Pos   = Pos[VertId];
    PSIn.Color = Col[VertId];
}
)"
};

const std::string DrawTest_PS{
R"(
struct PSInput 
{ 
    float4 Pos   : SV_POSITION; 
    float3 Color : COLOR; 
};

float4 main(in PSInput PSIn) : SV_Target
{
    return float4(PSIn.Color.rgb, 1.0);
}
)"
};

const std::string InputAttachmentTest_PS{
R"(

Texture2D<float4> g_SubpassInput;
SamplerState      g_SubpassInput_sampler;

struct PSInput 
{ 
    float4 Pos   : SV_POSITION; 
    float3 Color : COLOR; 
};

float4 main(in PSInput PSIn) : SV_Target
{
    float4 Color;
    Color.rgb = PSIn.Color.rgb * 0.125;
    Color.rgb += (float3(1.0, 1.0, 1.0) - g_SubpassInput.Load(int3(PSIn.Pos.xy, 0)).brg) * 0.875;
    Color.a = 1.0;
    return Color;
}
)"
};

// clang-format on

} // namespace HLSL

} // namespace
