
RWTexture2D<unorm float4> g_tex2DTestUAV;

[numthreads(16,16,1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint2 ui2Dim;
	g_tex2DTestUAV.GetDimensions(ui2Dim.x, ui2Dim.y);
	if( DTid.x >= ui2Dim.x || DTid.y >= ui2Dim.y )return;

	float2 f2UV = float2(DTid.xy) / float2(ui2Dim);
	float DistFromCenter = length(f2UV - float2(0.5,0.5));
	g_tex2DTestUAV[DTid.xy] = float4((1-DistFromCenter), abs(f2UV.x-0.5), abs(0.5-f2UV.y), 0.0);
}
