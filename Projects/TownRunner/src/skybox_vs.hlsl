cbuffer SkyboxConstantBuffer// : register(b0)
{
    float4x4 mViewProjection;
};

struct Skybox_VSOut
{
	float3 coords   : UVFACE;
};

void skybox_vs(	in float3 in_position : ATTRIB0,
            	in float3 uvFace   : ATTRIB1,
                out float4 position : SV_Position,
                out Skybox_VSOut vsoutput)
{
    // NOTE: Don't translate skybox and make sure depth == 1 (no clipping)
    position = mul(mViewProjection, float4(in_position, 0.0f)).xyww;
    position.z = 0.0f; // 1-z
    vsoutput.coords = in_position.xyz;
}
