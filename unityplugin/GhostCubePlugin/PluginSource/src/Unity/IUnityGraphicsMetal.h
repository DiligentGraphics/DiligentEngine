#pragma once
#include "IUnityInterface.h"

#ifndef __OBJC__
	#error metal plugin is objc code.
#endif
#ifndef __clang__
	#error only clang compiler is supported.
#endif
#if !__has_feature(objc_arc)
	#error metal demands ARC enabled.
#endif

@class NSBundle;
@protocol MTLDevice;
@protocol MTLCommandBuffer;
@protocol MTLCommandEncoder;
@protocol MTLTexture;

struct RenderSurfaceBase;
typedef struct RenderSurfaceBase* UnityRenderBuffer;

// Should only be used on the rendering thread unless noted otherwise.
UNITY_DECLARE_INTERFACE(IUnityGraphicsMetal)
{
	NSBundle*				(UNITY_INTERFACE_API * MetalBundle)();
	id<MTLDevice> 			(UNITY_INTERFACE_API * MetalDevice)();

	id<MTLCommandBuffer>	(UNITY_INTERFACE_API * CurrentCommandBuffer)();

	// for custom rendering support there are two scenarios:
	// you want to use current in-flight MTLCommandEncoder (NB: it might be nil)
	id<MTLCommandEncoder>	(UNITY_INTERFACE_API * CurrentCommandEncoder)();
	// or you might want to create your own encoder.
	// In that case you should end unity's encoder before creating your own and end yours before returning control to unity
	void					(UNITY_INTERFACE_API * EndCurrentCommandEncoder)();

	// access to RenderBuffer's texure
	// NB: you pass here *native* RenderBuffer, acquired by calling (C#) RenderBuffer.GetNativeRenderBufferPtr
	// AAResolvedTextureFromRenderBuffer will return nil in case of non-AA RenderBuffer or if called for depth RenderBuffer
	// StencilTextureFromRenderBuffer will return nil in case of no-stencil RenderBuffer or if called for color RenderBuffer
	id<MTLTexture>			(UNITY_INTERFACE_API * TextureFromRenderBuffer)(UnityRenderBuffer buffer);
	id<MTLTexture>			(UNITY_INTERFACE_API * AAResolvedTextureFromRenderBuffer)(UnityRenderBuffer buffer);
	id<MTLTexture>			(UNITY_INTERFACE_API * StencilTextureFromRenderBuffer)(UnityRenderBuffer buffer);

};
UNITY_REGISTER_INTERFACE_GUID(0x992C8EAEA95811E5ULL,0x9A62C4B5B9876117ULL,IUnityGraphicsMetal)
