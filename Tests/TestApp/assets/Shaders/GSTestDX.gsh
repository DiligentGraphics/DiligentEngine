
struct VSOutput
{
	float4 f4Position : SV_Position;
	float3 f3Color	: COLOR;
};

struct GSOut
{
    VSOutput VSOut;
};


[maxvertexcount(6)]
void main(point VSOutput In[1], 
          inout TriangleStream<GSOut> triStream )
{
    float2 Offsets[3];
    Offsets[0] = float2(-0.05, -0.05);
    Offsets[1] = float2(+0.05, -0.05);
    Offsets[2] = float2( 0.0, 0.0);
    for(int i=0; i<3; i++)
    {
        GSOut Out;
        Out.VSOut = In[0];
        Out.VSOut.f4Position.xy += Offsets[i];
        triStream.Append( Out );
    }
    triStream.RestartStrip();
    for(int j=0; j<3; j++)
    {
        GSOut Out;
        Out.VSOut = In[0];
        Out.VSOut.f4Position.xy += Offsets[j]*float2(1.0, -1.0);
        triStream.Append( Out );
    }
}
