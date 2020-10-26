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

#include "font.h"

#include <vector>
#include <string>


class GUIControl
{
protected:
    int mX = 0;
    int mY = 0;
    int mWidth  = 0;
    int mHeight = 0;
    std::string mTextureFile = "";
    bool mVisible = true;;

public:
    GUIControl() {}
    virtual ~GUIControl() {}

    // For now, empty string means use the font texture/shader... let's not overengineer this yet :)
    std::string TextureFile() const { return mTextureFile; }

    void Visible(bool visible) { mVisible = visible; }
    bool Visible() const { return mVisible; }

    virtual SpriteVertex* Draw(float viewportWidth, float viewportHeight, SpriteVertex* outVertex) const = 0;

    bool HitTest(int x, int y) const
    {
        return mVisible && (x >= mX && x < (mX + mWidth) && y > mY && y < (mY + mHeight));
    }
};


class GUIText : public GUIControl
{
private:
    std::string mText;
    const BitmapFont* mFont;

    void ComputeDimensions()
    {
        mFont->GetDimensions(mText.c_str(), &mWidth, &mHeight);
    }

public:
    // Font lifetime managed by caller
    GUIText(int x, int y, const BitmapFont* font, const std::string& text)
        : mFont(font), mText(text)
    {
        mX = x;
        mY = y;
        ComputeDimensions();
    }

    void Text(const std::string& text)
    {
        mText = text;
        ComputeDimensions();
    }

    virtual SpriteVertex* Draw(float viewportWidth, float viewportHeight, SpriteVertex* outVertex) const override
    {
        return mFont->DrawString(mText.c_str(), float(mX), float(mY), viewportWidth, viewportHeight, outVertex);
    }
};


class GUISprite : public GUIControl
{
private:
    std::string mSpriteFile;
    
public:
    GUISprite(int x, int y, int width, int height, const std::string& spriteFile)
        : mSpriteFile(spriteFile)
    {
        mX = x;
        mY = y;
        mWidth = width;
        mHeight = height;
        mTextureFile = spriteFile;
    }
    
    virtual SpriteVertex* Draw(float viewportWidth, float viewportHeight, SpriteVertex* outVertex) const override
    {
        return DrawSprite(float(mX), float(mY), float(mWidth), float(mHeight), viewportWidth, viewportHeight, outVertex);
    }
};


class GUI
{
private:
    std::vector<GUIControl*> mControls;
    IntelClearBold mFont; // Single font for the entire GUI works for now

public:
    GUI()
    {
    }
    ~GUI()
    {
        for (auto i : mControls) {
            delete i;
        }
    }
    
    const BitmapFont* Font() const { return &mFont; }

    GUIText* AddText(int x, int y, const std::string& text = "")
    {
        auto control = new GUIText(x, y, &mFont, text);
        mControls.push_back(control);
        return control;
    }

    GUISprite* AddSprite(int x, int y, int width, int height, const std::string& spriteFile)
    {
        auto control = new GUISprite(x, y, width, height, spriteFile);
        mControls.push_back(control);
        return control;
    }

    // NOTE: Caller needs to preadjust x/y for any stretching/scaling going on
    GUIControl* HitTest(int x, int y) const
    {
        for (auto i : mControls) {
            if (i->HitTest(x, y)) return i;
        }
        return nullptr;
    }

    GUIControl* operator[](size_t i) { return mControls[i]; }
    size_t size() const { return mControls.size(); }
};
