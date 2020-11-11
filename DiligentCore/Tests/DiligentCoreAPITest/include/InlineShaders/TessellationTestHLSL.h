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
const std::string TessTest_VS{
R"(
struct VSOutput
{
    float4 f4Position : SV_Position;
    float3 f3Color    : COLOR;
};

void main(uint uiVertexId : SV_VertexID,
          out VSOutput Out)
{
    float4 Positions[2];
    Positions[0] = float4(-0.5, -0.5, 0.0, 1.0);
    Positions[1] = float4(+0.5, +0.5, 0.0, 1.0);

    float3 Color[2];
    Color[0] = float3(0.5, 0.0, 0.0);
    Color[1] = float3(0.0, 0.0, 0.5);

    Out.f4Position = Positions[uiVertexId];
    Out.f3Color    = Color[uiVertexId];
}
)"
};

const std::string TessTest_HS{
R"(
struct VSOutput
{
    float4 f4Position : SV_Position;
    float3 f3Color	  : COLOR;
};

struct HS_CONSTANT_DATA_OUTPUT
{
    float Edges[4]  : SV_TessFactor;
    float Inside[2] : SV_InsideTessFactor;
};

HS_CONSTANT_DATA_OUTPUT ConstantHS(InputPatch<VSOutput, 1> p, 
                                   uint BlockID        : SV_PrimitiveID)
{
    HS_CONSTANT_DATA_OUTPUT Factors;
    Factors.Edges[0] = 2.5;
    Factors.Edges[1] = 4.25;
    Factors.Edges[2] = 5.75;
    Factors.Edges[3] = 7.5;

    Factors.Inside[0] = 6.75;
    Factors.Inside[1] = 7.25;

    return Factors;
}

struct HSOutput
{
    float4 Position : POS;
    float3 Color    : COL;
};

[domain("quad")]
[partitioning("fractional_even")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(1)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor( 34.0 )]
HSOutput main(InputPatch<VSOutput, 1> inputPatch, uint uCPID : SV_OutputControlPointID)
{
    HSOutput Out;
    Out.Position = inputPatch[uCPID].f4Position;
    Out.Color    = inputPatch[uCPID].f3Color;
    return Out;
}
)"
};


const std::string TessTest_DS{
R"(
struct DSOutput
{
    float4 f4Position : SV_Position;
    float3 f3Color	  : COLOR;
};

struct HS_CONSTANT_DATA_OUTPUT
{
    float Edges[4]  : SV_TessFactor;
    float Inside[2] : SV_InsideTessFactor;
};

struct HSOutput
{
    float4 Position : POS;
    float3 Color    : COL;
};

[domain("quad")]
/* partitioning = fractional_even, 
   outputtopology = triangle_ccw */
void main( HS_CONSTANT_DATA_OUTPUT Input, 
           float2 QuadUV : SV_DomainLocation, 
           OutputPatch<HSOutput, 1> QuadPatch,
           out DSOutput Out)
{
    Out.f4Position.xy = QuadPatch[0].Position.xy + (QuadUV.xy - float2(0.5, 0.5)) * 0.875;
    Out.f4Position.zw = QuadPatch[0].Position.zw;
    Out.f3Color = QuadPatch[0].Color + float3(QuadUV.xy, 1.0 - QuadUV.x * QuadUV.y) * 0.75;
}
)"
};

const std::string TessTest_PS{
R"(
struct DSOutput
{
    float4 f4Position : SV_Position;
    float3 f3Color    : COLOR;
};
 
void main(DSOutput In,
          out float4 Color : SV_Target)
{
    Color = float4(In.f3Color, 1.0);
}
)"
};
// clang-format on

} // namespace HLSL

} // namespace
