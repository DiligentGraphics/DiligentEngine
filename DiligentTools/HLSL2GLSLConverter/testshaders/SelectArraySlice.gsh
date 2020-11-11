#include "CommonShaderStructs.shinc"

struct SelectArraySliceGSOut
{
    QuadVSOut VSOutput;
    int RenderTargetIndex : SV_RenderTargetArrayIndex;
};

cbuffer cbTileAttribs
{
    int g_TileArrayIndex[4];
}

[maxvertexcount(3)]
void SelectArraySliceGS(triangle QuadVSOut In[3], 
                        inout TriangleStream<SelectArraySliceGSOut> triStream )
{
    uint InstID = In[0].m_uiInstID;
    int RTIndex = g_TileArrayIndex[InstID];
    for(int i=0; i<3; i++)
    {
        SelectArraySliceGSOut Out;
        Out.VSOutput = In[i];
        Out.RenderTargetIndex = RTIndex;
        triStream.Append( Out );
    }
}
