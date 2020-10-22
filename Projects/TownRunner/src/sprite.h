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

struct SpriteVertex {
    float x;
    float y;
    float u;
    float v;
};

inline SpriteVertex* DrawSprite(float x, float y, float width, float height, float viewportWidth, float viewportHeight, SpriteVertex* outVertex)
{
    outVertex[0] = {x        , y         , 0.0f, 0.0f};
    outVertex[1] = {x + width, y         , 1.0f, 0.0f};
    outVertex[2] = {x + width, y + height, 1.0f, 1.0f};
    outVertex[3] = outVertex[0];
    outVertex[4] = outVertex[2];
    outVertex[5] = {x        , y + height, 0.0f, 1.0f};

    for (int i = 0; i < 6; ++i) {
        outVertex[i].x =  (outVertex[i].x / viewportWidth  * 2.0f - 1.0f);
        outVertex[i].y = -(outVertex[i].y / viewportHeight * 2.0f - 1.0f);
    }

    outVertex += 6;
    return outVertex;
}
