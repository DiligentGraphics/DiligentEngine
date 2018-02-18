struct VSOut
{
	float4 f4Position : SV_Position;
	float2 f2UV	      : TEXCOORD;
};
 
void main(VSOut In,
          out float4 Col0 : SV_Target0,
          out float4 Col1 : SV_Target1)
{
    Col0 = float4( 0.5+0.5*cos(In.f2UV.x*3.1415*6.0), 0.0, 0.0, 0.0 );
    Col1 = float4( 0.0, 0.5+0.5*cos(In.f2UV.y*3.1415*8.0), 0.0, 0.0 );
}
