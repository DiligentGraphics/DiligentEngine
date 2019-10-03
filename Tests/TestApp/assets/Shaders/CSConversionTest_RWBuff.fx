RWBuffer</* format = r32f */ float >      TexBuff_F;
RWBuffer</* format = rg16i */ int2 >      TexBuff_I;
RWBuffer</* format = rgba16ui */ uint4 >  TexBuff_U;


void TestGetDimensions()
{
    //RWBuffer
    {
        uint uWidth;
        int iWidth;
        float fWidth;
        TexBuff_F.GetDimensions(uWidth);
        TexBuff_I.GetDimensions(iWidth);
        TexBuff_U.GetDimensions(fWidth);
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
}



void TestStore()
{
    int4 Location = int4(2, 5, 1, 10);

    //Buffer
    {
        TexBuff_F[Location.x] = 1.0;
        TexBuff_I[Location.x] = int2(1,2);
        TexBuff_U[Location.x] = uint4(1,2,3,4);
    }
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
    TestStore();
}
