cbuffer Constants
{
    float4x4 g_ViewProj;
    float4x4 g_Rotation;
};

cbuffer InstanceData
{
    float4x4 g_InstanceMatr;
};

struct VSInput
{
    float3 Pos : ATTRIB0; 
    float2 UV  : ATTRIB1;
};

struct PSInput 
{ 
    float4 Pos : SV_POSITION; 
    float2 UV : TEX_COORD; 
};

// By convention, Diligent Engine expects vertex shader inputs to be labeled as ATTRIBn, where n is the attribute number.
// Note that if separate shader objects are not supported (this is only the case for old GLES3.0 devices), vertex
// shader output variable name must match exactly the name of the pixel shader input variable.
// If the variable has structure type (like in this example), the structure declarations must also be indentical.
void main(in  VSInput VSIn,
          out PSInput PSIn) 
{
    // Apply rotation
    float4 TransformedPos = mul( float4(VSIn.Pos,1.0),g_Rotation);
    // Apply instance-specific transformation
    TransformedPos = mul(TransformedPos, g_InstanceMatr);
    // Apply view-projection matrix
    PSIn.Pos = mul( TransformedPos, g_ViewProj);
    PSIn.UV  = VSIn.UV;
}
