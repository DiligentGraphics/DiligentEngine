cbuffer ShaderConstants
{
    float4x4 g_CameraViewProj;
    float4x4 g_CameraViewInvProj;
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

    float3 Pos = VSIn.Pos * 0.9;
    Pos.x += float(GridX) * 2.0;
    Pos.y += float(GridY) * 2.0;
    Pos.z += float(GridX + GridY);
    
    PSIn.Pos = mul( float4(Pos, 1.0), g_CameraViewProj);
    PSIn.UV  = VSIn.UV;
}
