#include "structures.fxh"

cbuffer Constants
{
    float4x4 g_WorldViewProj;
    float4x4 g_NormalTranform;
    float4   g_LightDirection;
};

// Note that if separate shader objects are not supported (this is only the case for old GLES3.0 devices), vertex
// shader output variable name must match exactly the name of the pixel shader input variable.
// If the variable has structure type (like in this example), the structure declarations must also be indentical.
void main(in  CubeVSInput VSIn,
          out CubePSInput PSIn)
{
    PSIn.Pos = mul(float4(VSIn.Pos,1.0), g_WorldViewProj);
    float3 Normal = mul(float4(VSIn.Normal, 0.0), g_NormalTranform).xyz;
    PSIn.NdotL = saturate(dot(Normal.xyz, -g_LightDirection.xyz));
    PSIn.UV  = VSIn.UV;
}
