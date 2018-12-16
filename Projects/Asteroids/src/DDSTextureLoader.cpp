//--------------------------------------------------------------------------------------
// File: DDSTextureLoader.cpp
//
// Functions for loading a DDS texture without using D3DX
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
//#include "DXUT.h"
#include "DDSTextureLoader.h"
#include <d3d9.h> // For D3DFMT stuff

#include <assert.h>
#include <algorithm>
using std::max;
using std::min;

#define SAFE_DELETE_ARRAY(a) if(a) { delete[] a; a=nullptr; }
#define SAFE_RELEASE(a) if(a) { a->Release(); a=nullptr; }


// From DXUT
//--------------------------------------------------------------------------------------
// Helper functions to create SRGB formats from typeless formats and vice versa
//--------------------------------------------------------------------------------------
DXGI_FORMAT MAKE_SRGB( _In_ DXGI_FORMAT format )
{
    switch( format )
    {
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
            return DXGI_FORMAT_BC1_UNORM_SRGB;

        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
            return DXGI_FORMAT_BC2_UNORM_SRGB;

        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
            return DXGI_FORMAT_BC3_UNORM_SRGB;

        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
            return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
            return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;

        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
            return DXGI_FORMAT_BC7_UNORM_SRGB;
    };

    return format;
}


//--------------------------------------------------------------------------------------
HRESULT LoadTextureDataFromFile( __in_z const WCHAR* szFileName, BYTE** ppHeapData,
                                DDS_HEADER** ppHeader,
                                BYTE** ppBitData, UINT* pBitSize )
{
    // open the file
    HANDLE hFile = CreateFileW(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                               FILE_FLAG_SEQUENTIAL_SCAN, NULL );
    if( INVALID_HANDLE_VALUE == hFile )
        return HRESULT_FROM_WIN32( GetLastError() );

    // Get the file size
    LARGE_INTEGER FileSize = {0};
    GetFileSizeEx( hFile, &FileSize );

    // File is too big for 32-bit allocation, so reject read
    if( FileSize.HighPart > 0 )
    {
        CloseHandle( hFile );
        return E_FAIL;
    }

    // Need at least enough data to fill the header and magic number to be a valid DDS
    if( FileSize.LowPart < (sizeof(DDS_HEADER)+sizeof(DWORD)) )
    {
        CloseHandle( hFile );
        return E_FAIL;
    }

    // create enough space for the file data
    *ppHeapData = new BYTE[ FileSize.LowPart ];
    if( !( *ppHeapData ) )
    {
        CloseHandle( hFile );
        return E_OUTOFMEMORY;
    }

    // read the data in
    DWORD BytesRead = 0;
    if( !ReadFile( hFile, *ppHeapData, FileSize.LowPart, &BytesRead, NULL ) )
    {
        CloseHandle( hFile );
        SAFE_DELETE_ARRAY( *ppHeapData );
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    if( BytesRead < FileSize.LowPart )
    {
        CloseHandle( hFile );
        SAFE_DELETE_ARRAY( *ppHeapData );
        return E_FAIL;
    }

    // DDS files always start with the same magic number ("DDS ")
    DWORD dwMagicNumber = *( DWORD* )( *ppHeapData );
    if( dwMagicNumber != DDS_MAGIC )
    {
        CloseHandle( hFile );
        SAFE_DELETE_ARRAY( *ppHeapData );
        return E_FAIL;
    }

    DDS_HEADER* pHeader = reinterpret_cast<DDS_HEADER*>( *ppHeapData + sizeof( DWORD ) );

    // Verify header to validate DDS file
    if( pHeader->dwSize != sizeof(DDS_HEADER)
        || pHeader->ddspf.dwSize != sizeof(DDS_PIXELFORMAT) )
    {
        CloseHandle( hFile );
        SAFE_DELETE_ARRAY( *ppHeapData );
        return E_FAIL;
    }

    // Check for DX10 extension
    bool bDXT10Header = false;
    if ( (pHeader->ddspf.dwFlags & DDS_FOURCC)
        && (MAKEFOURCC( 'D', 'X', '1', '0' ) == pHeader->ddspf.dwFourCC) )
    {
        // Must be long enough for both headers and magic value
        if( FileSize.LowPart < (sizeof(DDS_HEADER)+sizeof(DWORD)+sizeof(DDS_HEADER_DXT10)) )
        {
            CloseHandle( hFile );
            SAFE_DELETE_ARRAY( *ppHeapData );
            return E_FAIL;
        }

        bDXT10Header = true;
    }

    // setup the pointers in the process request
    *ppHeader = pHeader;
    INT offset = sizeof( DWORD ) + sizeof( DDS_HEADER )
                 + (bDXT10Header ? sizeof( DDS_HEADER_DXT10 ) : 0);
    *ppBitData = *ppHeapData + offset;
    *pBitSize = FileSize.LowPart - offset;

    CloseHandle( hFile );

    return S_OK;
}


static UINT BitsPerPixel( DXGI_FORMAT fmt )
{
    switch( fmt )
    {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return 128;

    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
        return 96;

    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        return 64;

    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return 32;

    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
        return 16;

    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_A8_UNORM:
        return 8;

    case DXGI_FORMAT_R1_UNORM:
        return 1;

    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        return 4;

    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return 8;

    default:
        return 0;
    }
}


static void GetSurfaceInfo( UINT width, UINT height, DXGI_FORMAT fmt, UINT* pNumBytes, UINT* pRowBytes, UINT* pNumRows )
{
    UINT numBytes = 0;
    UINT rowBytes = 0;
    UINT numRows = 0;

    bool bc = false;
    bool packed  = false;
    UINT bcnumBytesPerBlock = 0;
    switch (fmt)
    {
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        bc=true;
        bcnumBytesPerBlock = 8;
        break;

    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        bc = true;
        bcnumBytesPerBlock = 16;
        break;

    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
        packed = true;
        break;
    }

    if( bc )
    {
        int numBlocksWide = 0;
        if( width > 0 )
            numBlocksWide = max( 1U, (width + 3) / 4 );
        int numBlocksHigh = 0;
        if( height > 0 )
            numBlocksHigh = max( 1U, (height + 3) / 4 );
        rowBytes = numBlocksWide * bcnumBytesPerBlock;
        numRows = numBlocksHigh;
    }
    else if ( packed )
    {
        rowBytes = ( ( width + 1 ) >> 1 ) * 4;
        numRows = height;
    }
    else
    {
        UINT bpp = BitsPerPixel( fmt );
        rowBytes = ( width * bpp + 7 ) / 8; // round up to nearest byte
        numRows = height;
    }

    numBytes = rowBytes * numRows;
    if( pNumBytes != NULL )
        *pNumBytes = numBytes;
    if( pRowBytes != NULL )
        *pRowBytes = rowBytes;
    if( pNumRows != NULL )
        *pNumRows = numRows;
}


//--------------------------------------------------------------------------------------
#define ISBITMASK( r,g,b,a ) ( ddpf.dwRBitMask == r && ddpf.dwGBitMask == g && ddpf.dwBBitMask == b && ddpf.dwABitMask == a )

//--------------------------------------------------------------------------------------
static D3DFORMAT GetD3D9Format( const DDS_PIXELFORMAT& ddpf )
{
    if( ddpf.dwFlags & DDS_RGB )
    {
        switch (ddpf.dwRGBBitCount)
        {
        case 32:
            if( ISBITMASK(0x00ff0000,0x0000ff00,0x000000ff,0xff000000) )
                return D3DFMT_A8R8G8B8;
            if( ISBITMASK(0x00ff0000,0x0000ff00,0x000000ff,0x00000000) )
                return D3DFMT_X8R8G8B8;
            if( ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0xff000000) )
                return D3DFMT_A8B8G8R8;
            if( ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0x00000000) )
                return D3DFMT_X8B8G8R8;

            // Note that many common DDS reader/writers (including D3DX) swap the
            // the RED/BLUE masks for 10:10:10:2 formats. We assumme
            // below that the 'incorrect' header mask is being used

            // For 'correct' writers this should be 0x3ff00000,0x000ffc00,0x000003ff for BGR data
            if( ISBITMASK(0x000003ff,0x000ffc00,0x3ff00000,0xc0000000) )
                return D3DFMT_A2R10G10B10;

            // For 'correct' writers this should be 0x000003ff,0x000ffc00,0x3ff00000 for RGB data
            if( ISBITMASK(0x3ff00000,0x000ffc00,0x000003ff,0xc0000000) )
                return D3DFMT_A2B10G10R10;

            if( ISBITMASK(0x0000ffff,0xffff0000,0x00000000,0x00000000) )
                return D3DFMT_G16R16;
            if( ISBITMASK(0xffffffff,0x00000000,0x00000000,0x00000000) )
                return D3DFMT_R32F; // D3DX writes this out as a FourCC of 114
            break;

        case 24:
            if( ISBITMASK(0x00ff0000,0x0000ff00,0x000000ff,0x00000000) )
                return D3DFMT_R8G8B8;
            break;

        case 16:
            if( ISBITMASK(0x0000f800,0x000007e0,0x0000001f,0x00000000) )
                return D3DFMT_R5G6B5;
            if( ISBITMASK(0x00007c00,0x000003e0,0x0000001f,0x00008000) )
                return D3DFMT_A1R5G5B5;
            if( ISBITMASK(0x00007c00,0x000003e0,0x0000001f,0x00000000) )
                return D3DFMT_X1R5G5B5;
            if( ISBITMASK(0x00000f00,0x000000f0,0x0000000f,0x0000f000) )
                return D3DFMT_A4R4G4B4;
            if( ISBITMASK(0x00000f00,0x000000f0,0x0000000f,0x00000000) )
                return D3DFMT_X4R4G4B4;

            // 3:3:2, 3:3:2:8, and paletted texture formats are typically not supported on modern video cards
            break;
        }
    }
    else if( ddpf.dwFlags & DDS_LUMINANCE )
    {
        if( 8 == ddpf.dwRGBBitCount )
        {
            if( ISBITMASK(0x0000000f,0x00000000,0x00000000,0x000000f0) )
                return D3DFMT_A4L4;
            if( ISBITMASK(0x000000ff,0x00000000,0x00000000,0x00000000) )
                return D3DFMT_L8;
        }

        if( 16 == ddpf.dwRGBBitCount )
        {
            if( ISBITMASK(0x0000ffff,0x00000000,0x00000000,0x00000000) )
                return D3DFMT_L16;
            if( ISBITMASK(0x000000ff,0x00000000,0x00000000,0x0000ff00) )
                return D3DFMT_A8L8;
        }
    }
    else if( ddpf.dwFlags & DDS_ALPHA )
    {
        if( 8 == ddpf.dwRGBBitCount )
        {
            return D3DFMT_A8;
        }
    }
    else if( ddpf.dwFlags & DDS_FOURCC )
    {
        if( MAKEFOURCC( 'D', 'X', 'T', '1' ) == ddpf.dwFourCC )
            return D3DFMT_DXT1;
        if( MAKEFOURCC( 'D', 'X', 'T', '2' ) == ddpf.dwFourCC )
            return D3DFMT_DXT2;
        if( MAKEFOURCC( 'D', 'X', 'T', '3' ) == ddpf.dwFourCC )
            return D3DFMT_DXT3;
        if( MAKEFOURCC( 'D', 'X', 'T', '4' ) == ddpf.dwFourCC )
            return D3DFMT_DXT4;
        if( MAKEFOURCC( 'D', 'X', 'T', '5' ) == ddpf.dwFourCC )
            return D3DFMT_DXT5;

        if( MAKEFOURCC( 'R', 'G', 'B', 'G' ) == ddpf.dwFourCC )
            return D3DFMT_R8G8_B8G8;
        if( MAKEFOURCC( 'G', 'R', 'G', 'B' ) == ddpf.dwFourCC )
            return D3DFMT_G8R8_G8B8;

        if( MAKEFOURCC( 'U', 'Y', 'V', 'Y' ) == ddpf.dwFourCC )
            return D3DFMT_UYVY;
        if( MAKEFOURCC( 'Y', 'U', 'Y', '2' ) == ddpf.dwFourCC )
            return D3DFMT_YUY2;

        // Check for D3DFORMAT enums being set here
        switch( ddpf.dwFourCC )
        {
        case D3DFMT_A16B16G16R16:
        case D3DFMT_Q16W16V16U16:
        case D3DFMT_R16F:
        case D3DFMT_G16R16F:
        case D3DFMT_A16B16G16R16F:
        case D3DFMT_R32F:
        case D3DFMT_G32R32F:
        case D3DFMT_A32B32G32R32F:
        case D3DFMT_CxV8U8:
            return (D3DFORMAT)ddpf.dwFourCC;
        }
    }

    return D3DFMT_UNKNOWN;
}

//--------------------------------------------------------------------------------------
DXGI_FORMAT GetDXGIFormat( const DDS_PIXELFORMAT& ddpf )
{
    if( ddpf.dwFlags & DDS_RGB )
    {
        switch (ddpf.dwRGBBitCount)
        {
        case 32:
            // DXGI_FORMAT_B8G8R8A8_UNORM_SRGB & DXGI_FORMAT_B8G8R8X8_UNORM_SRGB should be
            // written using the DX10 extended header instead since these formats require
            // DXGI 1.1
            //
            // This code will use the fallback to swizzle BGR to RGB in memory for standard
            // DDS files which works on 10 and 10.1 devices with WDDM 1.0 drivers
            //
            // NOTE: We don't use DXGI_FORMAT_B8G8R8X8_UNORM or DXGI_FORMAT_B8G8R8X8_UNORM
            // here because they were defined for DXGI 1.0 but were not required for D3D10/10.1

            if( ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0xff000000) )
                return DXGI_FORMAT_R8G8B8A8_UNORM;

            // No D3DFMT_X8B8G8R8 in DXGI. We'll deal with it in a swizzle case to ensure
            // alpha channel is 255 (don't care formats could contain garbage)

            // Note that many common DDS reader/writers (including D3DX) swap the
            // the RED/BLUE masks for 10:10:10:2 formats. We assumme
            // below that the 'backwards' header mask is being used since it is most
            // likely written by D3DX. The more robust solution is to use the 'DX10'
            // header extension and specify the DXGI_FORMAT_R10G10B10A2_UNORM format directly

            // For 'correct' writers, this should be 0x000003ff,0x000ffc00,0x3ff00000 for RGB data
            if( ISBITMASK(0x3ff00000,0x000ffc00,0x000003ff,0xc0000000) )
                return DXGI_FORMAT_R10G10B10A2_UNORM;

            if( ISBITMASK(0x0000ffff,0xffff0000,0x00000000,0x00000000) )
                return DXGI_FORMAT_R16G16_UNORM;

            if( ISBITMASK(0xffffffff,0x00000000,0x00000000,0x00000000) )
                // Only 32-bit color channel format in D3D9 was R32F
                return DXGI_FORMAT_R32_FLOAT; // D3DX writes this out as a FourCC of 114
            break;

        case 24:
            // No 24bpp DXGI formats
            break;

        case 16:
            // 5:5:5 & 5:6:5 formats are defined for DXGI, but are deprecated for D3D10, 10.0, and 11

            // No 4bpp, 3:3:2, 3:3:2:8, or paletted DXGI formats
            break;
        }
    }
    else if( ddpf.dwFlags & DDS_LUMINANCE )
    {
        if( 8 == ddpf.dwRGBBitCount )
        {
            if( ISBITMASK(0x000000ff,0x00000000,0x00000000,0x00000000) )
                return DXGI_FORMAT_R8_UNORM; // D3DX10/11 writes this out as DX10 extension

            // No 4bpp DXGI formats
        }

        if( 16 == ddpf.dwRGBBitCount )
        {
            if( ISBITMASK(0x0000ffff,0x00000000,0x00000000,0x00000000) )
                return DXGI_FORMAT_R16_UNORM; // D3DX10/11 writes this out as DX10 extension
            if( ISBITMASK(0x000000ff,0x00000000,0x00000000,0x0000ff00) )
                return DXGI_FORMAT_R8G8_UNORM; // D3DX10/11 writes this out as DX10 extension
        }
    }
    else if( ddpf.dwFlags & DDS_ALPHA )
    {
        if( 8 == ddpf.dwRGBBitCount )
        {
            return DXGI_FORMAT_A8_UNORM;
        }
    }
    else if( ddpf.dwFlags & DDS_FOURCC )
    {
        if( MAKEFOURCC( 'D', 'X', 'T', '1' ) == ddpf.dwFourCC )
            return DXGI_FORMAT_BC1_UNORM;
        if( MAKEFOURCC( 'D', 'X', 'T', '3' ) == ddpf.dwFourCC )
            return DXGI_FORMAT_BC2_UNORM;
        if( MAKEFOURCC( 'D', 'X', 'T', '5' ) == ddpf.dwFourCC )
            return DXGI_FORMAT_BC3_UNORM;

        // While pre-mulitplied alpha isn't directly supported by the DXGI formats,
        // they are basically the same as these BC formats so they can be mapped
        if( MAKEFOURCC( 'D', 'X', 'T', '2' ) == ddpf.dwFourCC )
            return DXGI_FORMAT_BC2_UNORM;
        if( MAKEFOURCC( 'D', 'X', 'T', '4' ) == ddpf.dwFourCC )
            return DXGI_FORMAT_BC3_UNORM;

        if( MAKEFOURCC( 'A', 'T', 'I', '1' ) == ddpf.dwFourCC )
            return DXGI_FORMAT_BC4_UNORM;
        if( MAKEFOURCC( 'B', 'C', '4', 'U' ) == ddpf.dwFourCC )
            return DXGI_FORMAT_BC4_UNORM;
        if( MAKEFOURCC( 'B', 'C', '4', 'S' ) == ddpf.dwFourCC )
            return DXGI_FORMAT_BC4_SNORM;

        if( MAKEFOURCC( 'A', 'T', 'I', '2' ) == ddpf.dwFourCC )
            return DXGI_FORMAT_BC5_UNORM;
        if( MAKEFOURCC( 'B', 'C', '5', 'U' ) == ddpf.dwFourCC )
            return DXGI_FORMAT_BC5_UNORM;
        if( MAKEFOURCC( 'B', 'C', '5', 'S' ) == ddpf.dwFourCC )
            return DXGI_FORMAT_BC5_SNORM;

        if( MAKEFOURCC( 'R', 'G', 'B', 'G' ) == ddpf.dwFourCC )
            return DXGI_FORMAT_R8G8_B8G8_UNORM;
        if( MAKEFOURCC( 'G', 'R', 'G', 'B' ) == ddpf.dwFourCC )
            return DXGI_FORMAT_G8R8_G8B8_UNORM;

        // Check for D3DFORMAT enums being set here
        switch( ddpf.dwFourCC )
        {
        case D3DFMT_A16B16G16R16: // 36
            return DXGI_FORMAT_R16G16B16A16_UNORM;

        case D3DFMT_Q16W16V16U16: // 110
            return DXGI_FORMAT_R16G16B16A16_SNORM;

        case D3DFMT_R16F: // 111
            return DXGI_FORMAT_R16_FLOAT;

        case D3DFMT_G16R16F: // 112
            return DXGI_FORMAT_R16G16_FLOAT;

        case D3DFMT_A16B16G16R16F: // 113
            return DXGI_FORMAT_R16G16B16A16_FLOAT;

        case D3DFMT_R32F: // 114
            return DXGI_FORMAT_R32_FLOAT;

        case D3DFMT_G32R32F: // 115
            return DXGI_FORMAT_R32G32_FLOAT;

        case D3DFMT_A32B32G32R32F: // 116
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        }
    }

    return DXGI_FORMAT_UNKNOWN;
}


//--------------------------------------------------------------------------------------
static HRESULT CreateTextureFromDDS( ID3D11Device* pDev, DDS_HEADER* pHeader, __inout_bcount(BitSize) BYTE* pBitData,
                                     UINT BitSize, __out ID3D11ShaderResourceView** ppSRV, bool bSRGB )
{
    HRESULT hr = S_OK;

    UINT iWidth = pHeader->dwWidth;
    UINT iHeight = pHeader->dwHeight;
    UINT iDepth = pHeader->dwDepth;

    UINT resDim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    UINT arraySize = 1;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    bool isCubeMap = false;

    UINT iMipCount = pHeader->dwMipMapCount;
    if( 0 == iMipCount )
        iMipCount = 1;

    bool swaprgb = false;
    bool seta = false;

    if ((pHeader->ddspf.dwFlags & DDS_FOURCC)
        && (MAKEFOURCC( 'D', 'X', '1', '0' ) == pHeader->ddspf.dwFourCC ) )
    {
        DDS_HEADER_DXT10* d3d10ext = (DDS_HEADER_DXT10*)( (char*)pHeader + sizeof(DDS_HEADER) );

        arraySize = d3d10ext->arraySize;
        if ( arraySize == 0 )
           return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );

        if ( BitsPerPixel( d3d10ext->dxgiFormat ) == 0 )
            return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
           
        format = d3d10ext->dxgiFormat;

        switch ( d3d10ext->resourceDimension )
        {
        case DDS_DIMENSION_TEXTURE1D:
            // D3DX writes 1D textures with a fixed Height of 1
            if ( (pHeader->dwFlags & DDS_HEIGHT) && iHeight != 1 )
                return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
            iHeight = iDepth = 1;
            break;

        case DDS_DIMENSION_TEXTURE2D:
            if ( d3d10ext->miscFlag & DDS_RESOURCE_MISC_TEXTURECUBE )
            {
                arraySize *= 6;
                isCubeMap = true;
            }
            iDepth = 1;
            break;

        case DDS_DIMENSION_TEXTURE3D:
            if ( !(pHeader->dwFlags & DDS_HEADER_FLAGS_VOLUME) )
                return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );

            if ( arraySize > 1 )
                return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
            break;

        default:
            return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
        }

        resDim = d3d10ext->resourceDimension;
    }
    else
    {
        format = GetDXGIFormat( pHeader->ddspf );

        if ( pHeader->dwFlags & DDS_HEADER_FLAGS_VOLUME )
        {
            resDim = DDS_DIMENSION_TEXTURE3D;
        }
        else 
        {
            if ( pHeader->dwCaps2 & DDS_CUBEMAP )
            {
               // We require all six faces to be defined
               if ( (pHeader->dwCaps2 & DDS_CUBEMAP_ALLFACES ) != DDS_CUBEMAP_ALLFACES )
                   return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );

                arraySize = 6;
                isCubeMap = true;
            }

            iDepth = 1;
            resDim = DDS_DIMENSION_TEXTURE2D;

            // Note there's no way for a legacy Direct3D 9 DDS to express a '1D' texture
        }

        if ( format == DXGI_FORMAT_UNKNOWN )
        {
            D3DFORMAT fmt = GetD3D9Format( pHeader->ddspf );

            switch( fmt )
            {
            case D3DFMT_X8R8G8B8:
                format = DXGI_FORMAT_B8G8R8A8_UNORM; // NOTE: DXGI1.1 requirement is fine here
                seta = true;
                break;

            case D3DFMT_A8R8G8B8:
                format = DXGI_FORMAT_B8G8R8A8_UNORM; // NOTE: DXGI1.1 requirement is fine here
                break;

            case D3DFMT_X8B8G8R8:
                format = DXGI_FORMAT_R8G8B8A8_UNORM;
                seta = true;
                break;

            case D3DFMT_A2R10G10B10:
                format = DXGI_FORMAT_R10G10B10A2_UNORM;
                swaprgb = TRUE;
                break;

            // Would need temp memory to expand other formats (24bpp, 4bpp, 16-bit (5:6:5, 5:5:5, 5:5:5:1), 3:3:2, 3:3:2:8)

            default:
                return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
            }
        }

        assert( BitsPerPixel( format ) != 0 );
    }

    // Bound sizes
    if ( iMipCount > D3D11_REQ_MIP_LEVELS )
        return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );

    switch ( resDim )
    {
        case DDS_DIMENSION_TEXTURE1D:
            if ( (arraySize > D3D11_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION)
                 || (iWidth > D3D11_REQ_TEXTURE1D_U_DIMENSION) )
                return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
            break;

        case DDS_DIMENSION_TEXTURE2D:
            if ( isCubeMap )
            {
                // This is the right bound because we set arraySize to (NumCubes*6) above
                if ( (arraySize > D3D11_REQ_TEXTURECUBE_DIMENSION)
                     || (iWidth > D3D11_REQ_TEXTURECUBE_DIMENSION) 
                     || (iHeight > D3D11_REQ_TEXTURECUBE_DIMENSION))
                    return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
            }
            else if ( (arraySize > D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION)
                 || (iWidth > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION) 
                 || (iHeight > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION))
            {
                return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
            }
            break;

        case DDS_DIMENSION_TEXTURE3D:
            if ( (arraySize > 1)
                 || (iWidth > D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION) 
                 || (iHeight > D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION)
                 || (iDepth > D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION) )
                return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
            break;
        }

    
    if ( bSRGB )
        format = MAKE_SRGB( format );

    // Create the texture
    D3D11_SUBRESOURCE_DATA* pInitData = new D3D11_SUBRESOURCE_DATA[ size_t{iMipCount} * size_t{arraySize} ];
    if( !pInitData )
        return E_OUTOFMEMORY;

    UINT NumBytes = 0;
    UINT RowBytes = 0;
    UINT NumRows = 0;
    BYTE* pSrcBits = pBitData;
    const BYTE *pEndBits = pBitData + BitSize;

    UINT index = 0;
    for( UINT j = 0; j < arraySize; j++ )
    {
        UINT w = iWidth;
        UINT h = iHeight;
        UINT d = iDepth;
        for( UINT i = 0; i < iMipCount; i++ )
        {
            GetSurfaceInfo( w, h, format, &NumBytes, &RowBytes, &NumRows );

            pInitData[index].pSysMem = ( void* )pSrcBits;
            pInitData[index].SysMemPitch = RowBytes;
            pInitData[index].SysMemSlicePitch = NumBytes;
            ++index;

            if ( pSrcBits + (size_t{NumBytes}*d) > pEndBits )
            {
                SAFE_DELETE_ARRAY( pInitData );
                return HRESULT_FROM_WIN32( ERROR_HANDLE_EOF );
            }

            if ( swaprgb || seta )
            {
                switch( format )
                {
                case DXGI_FORMAT_R8G8B8A8_UNORM:
                    {
                        BYTE *sptr = pSrcBits;
                        for ( UINT slice = 0; slice < d; ++slice )
                        {
                            BYTE *rptr = sptr;
                            for ( UINT row = 0; row < NumRows; ++row )
                            {
                                BYTE *ptr = rptr;
                                for( UINT x = 0; x < w; ++x, ptr += 4 )
                                {
                                    if ( ptr + 4 <= pEndBits )
                                    {
                                        if ( swaprgb )
                                        {
                                            BYTE a = ptr[0];
                                            ptr[0] = ptr[2];
                                            ptr[2] = a;
                                        }
                                        if ( seta )
                                            ptr[3] = 255;
                                    }
                                }
                                rptr += RowBytes;
                            }
                            sptr += NumBytes;
                        }           
                    }
                    break;

                case DXGI_FORMAT_R10G10B10A2_UNORM:
                    {
                        BYTE *sptr = pSrcBits;
                        for ( UINT slice = 0; slice < d; ++slice )
                        {
                            const BYTE *rptr = sptr;
                            for ( UINT row = 0; row < NumRows; ++row )
                            {
                                DWORD *ptr = (DWORD*)rptr;
                                for( UINT x = 0; x < w; ++x, ++ptr )
                                {
                                    if ( ptr + 1 <= (DWORD*)pEndBits )
                                    {
                                        DWORD t = *ptr;
                                        DWORD u = (t & 0x3ff00000) >> 20;
                                        DWORD v = (t & 0x000003ff) << 20;
                                        *ptr = ( t & ~0x3ff003ff ) | u | v; 
                                    }
                                }
                                rptr += RowBytes;
                            }
                            sptr += NumBytes;
                        }           
                    }
                    break;
                }
            }
    
            pSrcBits += size_t{NumBytes} * d;

            w = w >> 1;
            h = h >> 1;
            d = d >> 1;
            if( w == 0 )
                w = 1;
            if( h == 0 )
                h = 1;
            if( d == 0 )
                d = 1;
        }
    }

    switch ( resDim ) 
    {
        case DDS_DIMENSION_TEXTURE1D:
            {
                D3D11_TEXTURE1D_DESC desc;
                desc.Width = iWidth; 
                desc.MipLevels = iMipCount;
                desc.ArraySize = arraySize;
                desc.Format = format;
                desc.Usage = D3D11_USAGE_DEFAULT;
                desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                desc.CPUAccessFlags = 0;
                desc.MiscFlags = 0;

                ID3D11Texture1D* pTex = NULL;
                hr = pDev->CreateTexture1D( &desc, pInitData, &pTex );
                if( SUCCEEDED( hr ) && pTex )
                {
#if defined(DEBUG) || defined(PROFILE)
                    pTex->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof("DDSTextureLoader")-1, "DDSTextureLoader" );
#endif
                    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
                    memset( &SRVDesc, 0, sizeof( SRVDesc ) );
                    SRVDesc.Format = format;

                    if ( arraySize > 1 )
                    {
                        SRVDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE1DARRAY;
                        SRVDesc.Texture1DArray.MipLevels = desc.MipLevels;
                        SRVDesc.Texture1DArray.ArraySize = arraySize;
                    }
                    else
                    {
                        SRVDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE1D;
                        SRVDesc.Texture1D.MipLevels = desc.MipLevels;
                    }

                    hr = pDev->CreateShaderResourceView( pTex, &SRVDesc, ppSRV );
                    SAFE_RELEASE( pTex );
                }
            }
           break;

        case DDS_DIMENSION_TEXTURE2D:
            {
                D3D11_TEXTURE2D_DESC desc;
                desc.Width = iWidth;
                desc.Height = iHeight;
                desc.MipLevels = iMipCount;
                desc.ArraySize = arraySize;
                desc.Format = format;
                desc.SampleDesc.Count = 1;
                desc.SampleDesc.Quality = 0;
                desc.Usage = D3D11_USAGE_DEFAULT;
                desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                desc.CPUAccessFlags = 0;
                desc.MiscFlags = (isCubeMap) ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;

                ID3D11Texture2D* pTex = NULL;
                hr = pDev->CreateTexture2D( &desc, pInitData, &pTex );
                if( SUCCEEDED( hr ) && pTex )
                {
#if defined(DEBUG) || defined(PROFILE)
                    pTex->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof("DDSTextureLoader")-1, "DDSTextureLoader" );
#endif
                    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
                    memset( &SRVDesc, 0, sizeof( SRVDesc ) );
                    SRVDesc.Format = format;

                    if ( isCubeMap )
                    {
                        if ( arraySize > 6 )
                        {
                            SRVDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURECUBEARRAY;
                            SRVDesc.TextureCubeArray.MipLevels = desc.MipLevels;

                            // Earlier we set arraySize to (NumCubes * 6)
                            SRVDesc.TextureCubeArray.NumCubes = arraySize / 6;
                        }
                        else
                        {
                            SRVDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURECUBE;
                            SRVDesc.TextureCube.MipLevels = desc.MipLevels;
                        }
                    }
                    else if ( arraySize > 1 )
                    {
                        SRVDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2DARRAY;
                        SRVDesc.Texture2DArray.MipLevels = desc.MipLevels;
                        SRVDesc.Texture2DArray.ArraySize = arraySize;
                    }
                    else
                    {
                        SRVDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
                        SRVDesc.Texture2D.MipLevels = desc.MipLevels;
                    }

                    hr = pDev->CreateShaderResourceView( pTex, &SRVDesc, ppSRV );
                    SAFE_RELEASE( pTex );
                }
            }
            break;

        case DDS_DIMENSION_TEXTURE3D:
            {
                D3D11_TEXTURE3D_DESC desc;
                desc.Width = iWidth;
                desc.Height = iHeight;
                desc.Depth = iDepth;
                desc.MipLevels = iMipCount;
                desc.Format = format;
                desc.Usage = D3D11_USAGE_DEFAULT;
                desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                desc.CPUAccessFlags = 0;
                desc.MiscFlags = 0;

                ID3D11Texture3D* pTex = NULL;
                hr = pDev->CreateTexture3D( &desc, pInitData, &pTex );
                if( SUCCEEDED( hr ) && pTex )
                {
#if defined(DEBUG) || defined(PROFILE)
                    pTex->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof("DDSTextureLoader")-1, "DDSTextureLoader" );
#endif
                    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
                    memset( &SRVDesc, 0, sizeof( SRVDesc ) );
                    SRVDesc.Format = format;
                    SRVDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE3D;
                    SRVDesc.Texture3D.MipLevels = desc.MipLevels;

                    hr = pDev->CreateShaderResourceView( pTex, &SRVDesc, ppSRV );
                    SAFE_RELEASE( pTex );
                }
            }
            break; 
    }

    SAFE_DELETE_ARRAY( pInitData );

    return hr;
}


//--------------------------------------------------------------------------------------
HRESULT CreateDDSTextureFromFile( __in ID3D11Device* pDev, __in_z const WCHAR* szFileName, __out_opt ID3D11ShaderResourceView** ppSRV, bool sRGB )
{
    if ( !pDev || !szFileName || !ppSRV )
        return E_INVALIDARG;

    BYTE* pHeapData = NULL;
    DDS_HEADER* pHeader = NULL;
    BYTE* pBitData = NULL;
    UINT BitSize = 0;

    HRESULT hr = LoadTextureDataFromFile( szFileName, &pHeapData, &pHeader, &pBitData, &BitSize );
    if(FAILED(hr))
    {
        SAFE_DELETE_ARRAY( pHeapData );
        return hr;
    }

    hr = CreateTextureFromDDS( pDev, pHeader, pBitData, BitSize, ppSRV, sRGB );
    SAFE_DELETE_ARRAY( pHeapData );

#if defined(DEBUG) || defined(PROFILE)
    if ( *ppSRV )
    {
        CHAR strFileA[MAX_PATH];
        WideCharToMultiByte( CP_ACP, 0, szFileName, -1, strFileA, MAX_PATH, NULL, FALSE );
        CHAR* pstrName = strrchr( strFileA, '\\' );
        if( pstrName == NULL )
            pstrName = strFileA;
        else
            pstrName++;
        
        (*ppSRV)->SetPrivateData( WKPDID_D3DDebugObjectName, lstrlenA(pstrName), pstrName );
    }
#endif

    return hr;
}
