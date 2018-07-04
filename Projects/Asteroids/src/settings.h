// Copyright 2014 Intel Corporation All Rights Reserved
//
// Intel makes no representations about the suitability of this software for any purpose.  
// THIS SOFTWARE IS PROVIDED ""AS IS."" INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES,
// EXPRESS OR IMPLIED, AND ALL LIABILITY, INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES,
// FOR THE USE OF THIS SOFTWARE, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY
// RIGHTS, AND INCLUDING THE WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
// Intel does not assume any responsibility for any errors which may appear in this software
// nor any responsibility to update it.

#pragma once

#include "camera.h"
#include "common_defines.h"

// Profiling
#define ENABLE_VTUNE_TASK_PROFILING 1

// Content settings
#ifdef _DEBUG
enum { NUM_ASTEROIDS = 2000 };
#else
enum { NUM_ASTEROIDS = 50000 };
#endif
enum { TEXTURE_DIM = 256 }; // Req'd to be pow2 at the moment
enum { TEXTURE_ANISO = 2 };
enum { NUM_UNIQUE_MESHES = 1000 };
enum { MESH_MAX_SUBDIV_LEVELS = 3 }; // 4x polys for each step.
// See common_defines.h for NUM_UNIQUE_TEXTURES (also needed by shader now)

#define SIM_ORBIT_RADIUS 450.f
#define SIM_DISC_RADIUS  120.f
#define SIM_MIN_SCALE    0.2f

// In FLIP swap chains the compositor owns one of your buffers at any given point
// Thus to run unconstrained (>vsync) frame rates, you need 3 buffers
enum { NUM_SWAP_CHAIN_BUFFERS = 5 };

// In D3D12, this is the pipeline depth in frames - larger = more latency, shorter = potential bubbles.
// Usually 2-4 are good values.
enum { NUM_FRAMES_TO_BUFFER = 3 };

// In D3D12 this is the number of command buffers we generate for the main scene rendering
// Also effectively max thread parallelism
enum { NUM_SUBSETS = 4 };

// Buffer size for dynamic sprite data
enum { MAX_SPRITE_VERTICES_PER_FRAME = 6 * 1024 };


// This structure is often copied/passed by value so don't put anything really expensive in it.
struct Settings
{
    double closeAfterSeconds = 0.0;

    // Will be scaled based on DPI at app start for convenience
    double renderScale = 1.0; // 1.0 = window dimensions, <1 = upscaling, >1 = supersampling    
    int windowWidth = 1080;
    int windowHeight = 720;
    int renderWidth;
    int renderHeight;

    int numThreads = 0; // 0 means #cpu-1
    unsigned int lockedFrameRate = 15;

    bool logFrameTimes = false;
    bool vsync = 0;

    bool windowed = true;
    enum RenderMode
    {
        NativeD3D11,
        NativeD3D12,
        DiligentD3D11,
        DiligentD3D12,
        DiligentVulkan
    }mode = DiligentD3D11;
       
    int resourceBindingMode = 2; // Only for DiligentD3D12 mode

    bool lockFrameRate = false;
    bool animate = true;

    // Multithreading actually makes debugging annoying so disable by default
#if defined(_DEBUG)
    bool multithreadedRendering = true;
#else
    bool multithreadedRendering = true;
#endif

    bool submitRendering = true;
    bool executeIndirect = false;
    bool warp = false;
};
