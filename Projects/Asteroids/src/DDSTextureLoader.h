//--------------------------------------------------------------------------------------
// File: DDSTextureLoader.h
//
// Functions for loading a DDS texture without using D3DX
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "DDS.h"
#include <d3d11.h>

HRESULT CreateDDSTextureFromFile( __in ID3D11Device* pDev, __in_z const WCHAR* szFileName, __out_opt ID3D11ShaderResourceView** ppSRV, bool sRGB = false );

// INTEL: Exposed this from the internals so we can load the data into D3D12, etc.
DXGI_FORMAT GetDXGIFormat( const DDS_PIXELFORMAT& ddpf );
HRESULT LoadTextureDataFromFile(__in_z const WCHAR* szFileName, BYTE** ppHeapData,
                                DDS_HEADER** ppHeader,
                                BYTE** ppBitData, UINT* pBitSize);
