#include "AtmosphereShadersCommon.fxh"

cbuffer cbPostProcessingAttribs
{
    EpipolarLightScatteringAttribs g_PPAttribs;
};

Texture2D<float2> g_tex2DCoordinates;
Texture2D<uint2>  g_tex2DInterpolationSource;

struct VSOutput
{
    float3 f3Color : COLOR;
    float2 f2PosXY : XY;
    float4 f4QuadCenterAndSize : QUAD_CENTER_SIZE;
};

void RenderSampleLocationsVS(in uint VertexID : SV_VertexID,
                             in uint InstID   : SV_InstanceID,
                             out VSOutput VSOut,
                             // IMPORTANT: non-system generated pixel shader input
                             // arguments must have the exact same name as vertex shader 
                             // outputs and must go in the same order.
                             // Moreover, even if the shader is not using the argument,
                             // it still must be declared.

                             out float4 f4PosPS : SV_Position)
{
    uint2 CoordTexDim;
    g_tex2DCoordinates.GetDimensions(CoordTexDim.x, CoordTexDim.y);
    uint2 TexelIJ = uint2( InstID%CoordTexDim.x, InstID/CoordTexDim.x );
    float2 f2QuadCenterPos = g_tex2DCoordinates.Load(int3(TexelIJ,0));

    uint2 ui2InterpolationSources = g_tex2DInterpolationSource.Load( int3(TexelIJ,0) );
    bool bIsInterpolation = ui2InterpolationSources.x != ui2InterpolationSources.y;

    float2 f2QuadSize = (bIsInterpolation ? 2.0 : 4.0) * g_PPAttribs.f4ScreenResolution.zw;
    float4 MinMaxUV = float4(f2QuadCenterPos.x-f2QuadSize.x, f2QuadCenterPos.y - f2QuadSize.y, f2QuadCenterPos.x+f2QuadSize.x, f2QuadCenterPos.y + f2QuadSize.y);
    
    float3 f3Color = bIsInterpolation ? float3(0.5,0.0,0.0) : float3(1.0,0.0,0.0);
    float4 Verts[4];
    Verts[0] = float4(MinMaxUV.xy, 1.0, 1.0);
    Verts[1] = float4(MinMaxUV.xw, 1.0, 1.0);
    Verts[2] = float4(MinMaxUV.zy, 1.0, 1.0);
    Verts[3] = float4(MinMaxUV.zw, 1.0, 1.0);

    f4PosPS = Verts[VertexID];
    VSOut.f2PosXY = Verts[VertexID].xy;
    VSOut.f3Color = f3Color;
    VSOut.f4QuadCenterAndSize = float4(f2QuadCenterPos, f2QuadSize);
}


void RenderSampleLocationsPS(in VSOutput VSOut,
                             out float4 f4Color : SV_Target)
{
    f4Color = float4(VSOut.f3Color, 1.0 - pow( length( (VSOut.f2PosXY - VSOut.f4QuadCenterAndSize.xy) / VSOut.f4QuadCenterAndSize.zw),4.0) );
}
