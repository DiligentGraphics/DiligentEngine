#include "structures.fxh"

cbuffer GSConstants
{
    GlobalConstants g_Constants;
};


[maxvertexcount(3)]
void TerrainGS(triangle TerrainDSOut In[3], 
               inout TriangleStream<TerrainGSOut> triStream )
{
    // Compute screen-space position of every vertex
    float2 v0 = g_Constants.ViewportSize.xy * In[0].Pos.xy / In[0].Pos.w;
    float2 v1 = g_Constants.ViewportSize.xy * In[1].Pos.xy / In[1].Pos.w;
    float2 v2 = g_Constants.ViewportSize.xy * In[2].Pos.xy / In[2].Pos.w;
    float2 edge0 = v2 - v1;
    float2 edge1 = v2 - v0;
    float2 edge2 = v1 - v0;
    // Compute triangle area
    float area = abs(edge1.x*edge2.y - edge1.y * edge2.x);

    TerrainGSOut Out;

    Out.DSOut = In[0];
    // Distance to edge0
    Out.DistToEdges = float3(area/length(edge0), 0.0, 0.0);
    triStream.Append( Out );

    Out.DSOut = In[1];
    // Distance to edge1
    Out.DistToEdges = float3(0.0, area/length(edge1), 0.0);
    triStream.Append( Out );

    Out.DSOut = In[2];
    // Distance to edge2
    Out.DistToEdges = float3(0.0, 0.0, area/length(edge2));
    triStream.Append( Out );
}
