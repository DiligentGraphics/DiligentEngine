
struct FullScreenTriangleVSOutput
{
    float4 f4PixelPos     : SV_Position;   // Pixel position on the screen
    float2 f2NormalizedXY : NORMALIZED_XY; // Normalized device XY coordinates [-1,1]x[-1,1]
    float  fInstID        : INSTANCE_ID;
};
