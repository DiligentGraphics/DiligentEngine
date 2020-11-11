RWBuffer</* format = r32f */ float >      TexBuff_F/*comment*/ : /*comment*/register(u0)/*comment*/;
RWBuffer</* format = rg16i */ int2 >      TexBuff_I;
RWBuffer</* format = rgba16ui */ uint4 >  TexBuff_U;

struct StorageBufferStruct
{
    float4 Data;
};

RWStructuredBuffer<StorageBufferStruct> RWStructBuff0 /*comment*/:/*comment*/ register(u1)/*comment*/;
RWStructuredBuffer<StorageBufferStruct> RWStructBuff1;
RWStructuredBuffer<StorageBufferStruct> RWStructBuff2 : register(u2);

void TestGetDimensions()
{
    //RWBuffer
    {
        uint uWidth;
        int iWidth;
        float fWidth;
        TexBuff_F.GetDimensions(uWidth);
        //TexBuff_I.GetDimensions(iWidth);
        //TexBuff_U.GetDimensions(fWidth);
    }
}


void TestLoad()
{
    int4 Location = int4(2, 5, 1, 10);

    //Buffer
    {
        TexBuff_F.Load(Location.x);
        TexBuff_I.Load(Location.x);
        TexBuff_U.Load(Location.x);
    }
    StorageBufferStruct Data0 = RWStructBuff0[Location.x];
    StorageBufferStruct Data1 = RWStructBuff1[Location.y];
    StorageBufferStruct Data3 = RWStructBuff2[Location.w];
}



void TestStore(uint3 Location)
{
    //Buffer
    {
        TexBuff_F[Location.x] = 1.0;
        TexBuff_I[Location.x] = int2(1,2);
        TexBuff_U[Location.x] = uint4(1,2,3,4);
    }
    StorageBufferStruct Data0;
    Data0.Data = float4(0.0, 1.0, 2.0, 3.0);
    RWStructBuff0[Location.x] = Data0;
    RWStructBuff1[Location.z] = Data0;
    RWStructBuff2[Location.y] = Data0;

}

struct CSInputSubstr
{
    uint3 DTid : SV_DispatchThreadID;
};
struct CSInput
{
    uint GroupInd : SV_GroupIndex;
    CSInputSubstr substr;
};

[numthreads(2,4,8)]
void TestCS(CSInput In,
            uint3 Gid : SV_GroupID,
            uint3 GTid : SV_GroupThreadID)
{
    TestGetDimensions();
    TestLoad();
    TestStore(GTid);
}
