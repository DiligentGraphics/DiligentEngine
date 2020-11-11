
#include "HostSharedTerrainStructs.fxh"
#include "TerrainShadersCommon.fxh"

Texture2D< uint > g_tex2DElevationMap;

cbuffer cbNMGenerationAttribs
{
    NMGenerationAttribs g_NMGenerationAttribs;
};


float3 ComputeNormal(int2 i2ElevMapIJ,
                     float fSampleSpacingInterval,
                     int MIPLevel)
{
    int MipWidth, MipHeight;
    // This version of GetDimensions() does not work on D3D12. Looks like a bug
    // in shader compiler
    //g_tex2DElevationMap.GetDimensions( MIPLevel, MipWidth, MipHeight, Levels );
    g_tex2DElevationMap.GetDimensions( MipWidth, MipHeight );
    MipWidth = MipWidth >> MIPLevel;
    MipHeight = MipHeight >> MIPLevel;

    int i0  = i2ElevMapIJ.x;
    int i1  = min( i0 + 1, MipWidth - 1 );
    int i_1 = max( i0 - 1, 0 );

    int j0  = i2ElevMapIJ.y;
    int j1  = min( j0 + 1, MipHeight - 1 );
    int j_1 = max( j0 - 1, 0 );

#   define GET_ELEV(i,j) float( g_tex2DElevationMap.Load(int3(i,j, MIPLevel)) )

#if 1
    float Height00 = GET_ELEV( i_1, j_1 );
    float Height10 = GET_ELEV(  i0, j_1 );
    float Height20 = GET_ELEV(  i1, j_1 );

    float Height01 = GET_ELEV( i_1, j0 );
  //float Height11 = GET_ELEV(  i0, j0 );
    float Height21 = GET_ELEV(  i1, j0 );

    float Height02 = GET_ELEV( i_1, j1 );
    float Height12 = GET_ELEV(  i0, j1 );
    float Height22 = GET_ELEV(  i1, j1 );

    float3 Grad;
    Grad.x = (Height00+Height01+Height02) - (Height20+Height21+Height22);
    Grad.y = (Height00+Height10+Height20) - (Height02+Height12+Height22);
    Grad.z = fSampleSpacingInterval * 6.0;
    //Grad.x = (3*Height00+10*Height01+3*Height02) - (3*Height20+10*Height21+3*Height22);
    //Grad.y = (3*Height00+10*Height10+3*Height20) - (3*Height02+10*Height12+3*Height22);
    //Grad.z = fSampleSpacingInterval * 32.f;
#else
    float Height1 = GET_ELEV(  i1,  j0 );
    float Height2 = GET_ELEV( i_1,  j0 );
    float Height3 = GET_ELEV(  i0,  j1 );
    float Height4 = GET_ELEV(  i0, j_1 );
       
    float3 Grad;
    Grad.x = Height2 - Height1;
    Grad.y = Height4 - Height3;
    Grad.z = fSampleSpacingInterval * 2.0;
#endif
    Grad.xy *= g_NMGenerationAttribs.m_fElevationScale;
    float3 Normal = normalize( Grad );

    return Normal;
}


void GenerateNormalMapPS(in float4 f4Pos : SV_Position,
                         out float2 f2outNormalXY : SV_Target)
{
    float3 Normal = ComputeNormal( int2(f4Pos.xy), g_NMGenerationAttribs.m_fSampleSpacingInterval*exp2( float(g_NMGenerationAttribs.m_iMIPLevel) ), g_NMGenerationAttribs.m_iMIPLevel );
    // Only xy components are stored. z component is calculated in the shader
    f2outNormalXY = Normal.xy * float2(0.5,0.5) + float2(0.5,0.5);
}
