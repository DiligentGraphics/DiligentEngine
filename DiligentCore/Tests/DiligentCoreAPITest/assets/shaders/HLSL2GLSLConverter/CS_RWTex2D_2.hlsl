RWTexture2D<int/* format = r32i */>Tex2D_I1: register(u0);
RWTexture2D<uint/* format = r32ui */>Tex2D_U1/*comment*/:/*comment*/register(u1)/*comment*/;

RWTexture2DArray <float// format = r32f 
                  >  Tex2D_F_A;
RWTexture2DArray <int2// format = rg8i 
                  >   Tex2D_I_A;
RWTexture2DArray <uint4// format = rgba8ui 
                  >  Tex2D_U_A;

RWTexture3D< float4/*
format
=
rgba32f
*/> Tex3D_F:register(u2);
RWTexture3D< int /* format = r8i */>    Tex3D_I;
RWTexture3D< uint2 /* format = rg8ui */>  Tex3D_U/*comment*/:/*comment*/
/*comment*/register(u3)/*comment*/;

void TestGetDimensions()
{
    //RWTexture2DArray
    {
        uint uWidth, uHeight, uElems;
        float fWidth, fHeight, fElems;
        int iWidth, iHeight, iElems;
        Tex2D_F_A.GetDimensions(uWidth,uHeight,uElems);
        Tex2D_U_A.GetDimensions( uWidth, uHeight, uElems );
        Tex2D_I_A.GetDimensions (uWidth , uHeight , uElems ) ;

        Tex2D_F_A.GetDimensions(iWidth,iHeight,iElems);
        Tex2D_U_A.GetDimensions( iWidth, iHeight, iElems );
        Tex2D_I_A.GetDimensions (iWidth , iHeight , iElems ) ;

        Tex2D_F_A.GetDimensions(fWidth,fHeight,fElems);
        Tex2D_U_A.GetDimensions( fWidth, fHeight, fElems );
        Tex2D_I_A.GetDimensions (fWidth , fHeight , fElems ) ;
    }

    //RWTexture3D
    {
        uint uWidth, uHeight, uDepth;
        int iWidth, iHeight, iDepth;
        float fWidth, fHeight, fDepth;
        Tex3D_F.GetDimensions ( uWidth, uHeight, uDepth );
        Tex3D_U.GetDimensions(uWidth,uHeight,uDepth);
        Tex3D_I.GetDimensions ( uWidth , uHeight , uDepth );

        Tex3D_F.GetDimensions ( iWidth, iHeight, iDepth );
        Tex3D_U.GetDimensions(iWidth,iHeight,iDepth);
        Tex3D_I.GetDimensions ( iWidth , iHeight , iDepth );

        Tex3D_F.GetDimensions ( fWidth, fHeight, fDepth );
        Tex3D_U.GetDimensions(fWidth,fHeight,fDepth);
        Tex3D_I.GetDimensions ( fWidth , fHeight , fDepth );
    }
}


void TestLoad()
{
    int4 Location = int4(2, 5, 1, 10);

    //Texture2DArray
    {
        float f  = Tex2D_F_A.Load(Location.xyz);
        uint4 u4 = Tex2D_U_A.Load(Location.xyz);
        int2 i2  = Tex2D_I_A.Load(Location.xyz);
    }

    //Texture3D
    {
        float4 f4 = Tex3D_F.Load(Location.xyz).xyzw;
        uint2  u2 = Tex3D_U.Load(Location.xyz).xy;
        int    i  = Tex3D_I.Load(Location.xyz);
    }
}



void TestStore(uint3 Location)
{
    //Texture2DArray
    {
        Tex2D_F_A[Location.xyz] = 30.0;
        Tex2D_U_A[Location.xyz] = uint4(0,5,7,87);
        Tex2D_I_A[Location.xyz] = int2(-3, 5);
    }

    //Texture3D
    {
        Tex3D_F[Location.xyz] = float4(10.0, 25.0, 26.0, 27.0);
        Tex3D_U[Location.xyz] = uint2(0,6);
        Tex3D_I[Location.xyz] = -5;
    }
}


int2 GetCoords( uint x, uint y )
{
    return int2(x, y);
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
