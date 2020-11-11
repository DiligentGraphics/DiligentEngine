cbuffer Constants
{
    float4x4 g_WorldViewProj;
};

// Vertex shader takes two inputs: vertex position and uv coordinates.
// By convention, Diligent Engine expects vertex shader inputs to be 
// labeled 'ATTRIBn', where n is the attribute number.
struct VSInput
{
    float3 Pos : ATTRIB0;
    float2 UV  : ATTRIB1;
};

struct PSInput 
{ 
    float4 Pos : SV_POSITION; 
    float2 UV  : TEX_COORD; 
};

// Note that if separate shader objects are not supported (this is only the case for old GLES3.0 devices), vertex
// shader output variable name must match exactly the name of the pixel shader input variable.
// If the variable has structure type (like in this example), the structure declarations must also be indentical.
void main(in  uint    InstID : SV_InstanceID,
          in  VSInput VSIn,
          out PSInput PSIn) 
{
    const uint GridDim = 7u;
    int GridX = int(InstID % GridDim) - int(GridDim) / 2;
    int GridY = int(InstID / GridDim) - int(GridDim) / 2;

    float3 Pos = VSIn.Pos;
    Pos.x += float(GridX) * 2.75;
    Pos.y += float(GridY) * 2.75;
    Pos.z *= 0.25;
    PSIn.Pos = mul( float4(Pos, 1.0), g_WorldViewProj);
    PSIn.UV  = (VSIn.UV - float2(0.5, 0.5)) * 0.9 + float2(0.5, 0.5);
}
