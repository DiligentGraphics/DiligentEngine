cbuffer ShaderConstants
{
    float4x4 g_ViewProj;
    float4x4 g_ViewProjInv;
    float4   g_ViewportSize;
    int      g_ShowLightVolumes;
};

// Vertex shader takes two inputs: vertex position and uv coordinates.
// By convention, Diligent Engine expects vertex shader inputs to be 
// labeled 'ATTRIBn', where n is the attribute number.
struct VSInput
{
    float3 Pos : ATTRIB0;
    float2 UV  : ATTRIB1;

    float4 LightLocation : ATTRIB2;
    float3 LightColor    : ATTRIB3;
};

struct PSInput
{ 
    float4 Pos           : SV_POSITION; 
    float4 LightLocation : LIGHT_LOCATION;
    float3 LightColor    : LIGHT_COLOR;
};

// Note that if separate shader objects are not supported (this is only the case for old GLES3.0 devices), vertex
// shader output variable name must match exactly the name of the pixel shader input variable.
// If the variable has structure type (like in this example), the structure declarations must also be indentical.
void main(in  uint    InstID : SV_InstanceID,
          in  VSInput VSIn,
          out PSInput PSIn) 
{
    float3 Pos = VSIn.LightLocation.xyz + VSIn.Pos.xyz * VSIn.LightLocation.w;
    PSIn.Pos = mul( float4(Pos, 1.0), g_ViewProj);

    PSIn.LightLocation = VSIn.LightLocation;
    PSIn.LightColor    = VSIn.LightColor;
}
