/*     Copyright 2015-2018 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF ANY PROPRIETARY RIGHTS.
 *
 *  In no event and under no legal theory, whether in tort (including negligence), 
 *  contract, or otherwise, unless required by applicable law (such as deliberate 
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental, 
 *  or consequential damages of any character arising as a result of this License or 
 *  out of the use or inability to use the software (including but not limited to damages 
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and 
 *  all other commercial damages or losses), even if such Contributor has been advised 
 *  of the possibility of such damages.
 */

#pragma once

#include "UnitTestBase.h"

class TestCreateObjFromNativeRes : public UnitTestBase
{
public:
    TestCreateObjFromNativeRes() : UnitTestBase("Object initialization from native resource handle test") {}
    virtual ~TestCreateObjFromNativeRes()
    {
        if(m_NumTexturesCreated != 0 || m_NumBuffersCreated != 0 )
        {
            std::stringstream infoss;
            infoss << "Textures crated: " << m_NumTexturesCreated <<" Buffers created: " << m_NumBuffersCreated;
            SetStatus(TestResult::Succeeded, infoss.str().c_str());
        }
        else
            SetStatus(TestResult::Skipped);
    }
    virtual void CreateTexture(Diligent::ITexture *pTexture) = 0;
    virtual void CreateBuffer(Diligent::IBuffer *pBuffer) = 0;
protected:
    int m_NumTexturesCreated = 0;
    int m_NumBuffersCreated = 0;
};
