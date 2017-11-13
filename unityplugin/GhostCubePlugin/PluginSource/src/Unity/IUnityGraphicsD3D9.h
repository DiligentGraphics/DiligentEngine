#pragma once
#include "IUnityInterface.h"

// Should only be used on the rendering thread unless noted otherwise.
UNITY_DECLARE_INTERFACE(IUnityGraphicsD3D9)
{
	IDirect3D9* (UNITY_INTERFACE_API * GetD3D)();
	IDirect3DDevice9* (UNITY_INTERFACE_API * GetDevice)();
};
UNITY_REGISTER_INTERFACE_GUID(0xE90746A523D53C4CULL,0xAC825B19B6F82AC3ULL,IUnityGraphicsD3D9)
