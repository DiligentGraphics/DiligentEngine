

float4 LUTCoordsFromThreadID( uint3 ThreadId )
{
    float4 f4Corrds;
    f4Corrds.xy = (float2( ThreadId.xy ) + float2(0.5,0.5) ) / PRECOMPUTED_SCTR_LUT_DIM.xy;

    uint uiW = ThreadId.z % uint(PRECOMPUTED_SCTR_LUT_DIM.z);
    uint uiQ = ThreadId.z / uint(PRECOMPUTED_SCTR_LUT_DIM.z);
    f4Corrds.zw = ( float2(uiW, uiQ) + float2(0.5,0.5) ) / PRECOMPUTED_SCTR_LUT_DIM.zw;
    return f4Corrds;
}
