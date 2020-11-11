
cbuffer Constants
{
    float4x4 g_MVP;
    float4x4 g_ModelMat;
    float4   g_Normal;
    float4   g_Color;
}

struct VSInput
{
    float3 Pos : ATTRIB0;
};

struct PSInput
{ 
    float4 Pos   : SV_POSITION; 
    float2 UV    : TEX_COORD; 
    float  Alpha : ALPHA;
};

void main(in  VSInput VSIn,
          out PSInput PSIn)
{
    // Vertex Z value is used as the alpha in this shader.
    PSIn.Alpha = VSIn.Pos.z;

    float4 LocalPos = float4(VSIn.Pos.x, 0.0, VSIn.Pos.y, 1.0);
    PSIn.Pos = mul(LocalPos, g_MVP);

    float4 WorldPos = mul(LocalPos, g_ModelMat);

    // Construct two vectors that are orthogonal to the normal.
    // This arbitrary choice is not co-linear with either horizontal
    // or vertical plane normals.
    float3 arbitrary = float3(1.0, 1.0, 0.0);
    float3 float_u = normalize(cross(g_Normal.xyz, arbitrary));
    float3 float_v = normalize(cross(g_Normal.xyz, float_u));

    // Project vertices in world frame onto float_u and float_v.
    PSIn.UV = float2(dot(WorldPos.xyz, float_u),
                     dot(WorldPos.xyz, float_v));
}
