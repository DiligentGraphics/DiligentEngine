#ifndef _HLSL_DEFINITIONS_
#define _HLSL_DEFINITIONS_

#define HLSL

#define NDC_MIN_Z 0.0 // Minimal z in the normalized device space

#define F3NDC_XYZ_TO_UVD_SCALE float3(0.5, -0.5, 1.0)

float2 NormalizedDeviceXYToTexUV( float2 f2ProjSpaceXY )
{
    return float2(0.5,0.5) + float2(0.5,-0.5) * f2ProjSpaceXY.xy;
}

float2 TexUVToNormalizedDeviceXY( float2 TexUV)
{
    return (TexUV.xy - float2(0.5, 0.5)) * float2(2.0, -2.0);
}

float NormalizedDeviceZToDepth(float fNDC_Z)
{
    return fNDC_Z;
}

float DepthToNormalizedDeviceZ(float fDepth)
{
    return fDepth;
}

// Relational and logical operators
#define Less(x,y) ((x)<(y))
#define LessEqual(x,y) ((x)<=(y))
#define Greater(x,y) ((x)>(y))
#define GreaterEqual(x,y) ((x)>=(y))
#define Equal(x,y) ((x)==(y))
#define NotEqual(x,y) ((x)!=(y))
#define Not(x) (!(x))
#define And(x,y) ((x)&&(y))
#define Or(x,y) ((x)||(y))

float4 BoolToFloat( bool4 b4 )
{
    return float4(b4.x ? 1.0 : 0.0,
                  b4.y ? 1.0 : 0.0,
                  b4.z ? 1.0 : 0.0,
                  b4.w ? 1.0 : 0.0);
}
float3 BoolToFloat( bool3 b3 )
{
    return float3(b3.x ? 1.0 : 0.0,
                  b3.y ? 1.0 : 0.0,
                  b3.z ? 1.0 : 0.0);
}
float2 BoolToFloat( bool2 b2 )
{
    return float2(b2.x ? 1.0 : 0.0,
                  b2.y ? 1.0 : 0.0);
}
float BoolToFloat( bool b )
{
    return b.x ? 1.0 : 0.0;
}

#define MATRIX_ELEMENT(mat, row, col) mat[row][col]

float4x4 MatrixFromRows(float4 row0, float4 row1, float4 row2, float4 row3)
{
    return float4x4(row0, row1, row2, row3);
}

float3x3 MatrixFromRows(float3 row0, float3 row1, float3 row2)
{
    return float3x3(row0, row1, row2);
}

float2x2 MatrixFromRows(float2 row0, float2 row1)
{
    return float2x2(row0, row1);
}

#endif // _HLSL_DEFINITIONS_

