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

#include "simplexnoise1234.h"

// Very simple multi-octave simplex noise helper
// Returns noise in the range [0, 1] vs. the usual [-1, 1]
template <size_t N = 4>
class NoiseOctaves
{
private:
    float mWeights[N];
    float mWeightNorm;
    
public:
    NoiseOctaves(float persistence = 0.5f)
    {
        float weightSum = 0.0f;
        for (size_t i = 0; i < N; ++i) {
            mWeights[i] = persistence;
            weightSum += persistence;
            persistence *= persistence;
        }
        mWeightNorm = 0.5f / weightSum; // Will normalize to [-0.5, 0.5]
    }

    // Returns [0, 1]
    float operator()(float x, float y, float z) const
    {
        float r = 0.0f;
        for (size_t i = 0; i < N; ++i) {
            r += mWeights[i] * snoise3(x, y, z);
            x *= 2.0f; y *= 2.0f; z *= 2.0f;
        }
        return r * mWeightNorm + 0.5f;
    }

    // Returns [0, 1]
    float operator()(float x, float y, float z, float w) const
    {
        float r = 0.0f;
        for (size_t i = 0; i < N; ++i) {
            r += mWeights[i] * snoise4(x, y, z, w);
            x *= 2.0f; y *= 2.0f; z *= 2.0f; w *= 2.0f;
        }
        return r * mWeightNorm + 0.5f;
    }
};