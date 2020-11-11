#include "structures.fxh"

cbuffer VSConstants
{
    GlobalConstants g_Constants;
};

struct TerrainVSIn
{
    uint BlockID : SV_VertexID;
};

void TerrainVS(in  TerrainVSIn  VSIn,
               out TerrainVSOut VSOut)
{
    uint BlockHorzOrder = VSIn.BlockID % g_Constants.NumHorzBlocks;
    uint BlockVertOrder = VSIn.BlockID / g_Constants.NumHorzBlocks;
    
    float2 BlockOffset = float2( 
        float(BlockHorzOrder) / g_Constants.fNumHorzBlocks,
        float(BlockVertOrder) / g_Constants.fNumVertBlocks
    );

    VSOut.BlockOffset = BlockOffset;
}
