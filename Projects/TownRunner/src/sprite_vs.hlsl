
struct Font_VSOut
{
	float2 uv : UV;
};

void sprite_vs(	float2 in_position : ATTRIB0,
            	float2 in_uv       : ATTRIB1,
                out float4 position : SV_Position,
                out Font_VSOut vs_out)
{
    position = float4(in_position, 0, 1);
    vs_out.uv = in_uv;
}
