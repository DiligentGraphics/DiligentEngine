/*
 *  Copyright 2019-2020 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
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

#include "pch.h"
#include "TextureUtilities.h"
#include "Errors.hpp"
#include "TextureLoader.h"
#include "Image.h"
#include "RefCntAutoPtr.hpp"
#include "DataBlobImpl.hpp"

namespace Diligent
{

void CreateTextureFromFile(const Char*            FilePath,
                           const TextureLoadInfo& TexLoadInfo,
                           IRenderDevice*         pDevice,
                           ITexture**             ppTexture)
{
    RefCntAutoPtr<Image>     pImage;
    RefCntAutoPtr<IDataBlob> pRawData;

    auto ImgFmt = CreateImageFromFile(FilePath, &pImage, &pRawData);

    if (pImage)
        CreateTextureFromImage(pImage, TexLoadInfo, pDevice, ppTexture);
    else if (pRawData)
    {
        if (ImgFmt == IMAGE_FILE_FORMAT_DDS)
            CreateTextureFromDDS(pRawData, TexLoadInfo, pDevice, ppTexture);
        else if (ImgFmt == IMAGE_FILE_FORMAT_KTX)
            CreateTextureFromKTX(pRawData, TexLoadInfo, pDevice, ppTexture);
        else
            UNEXPECTED("Unexpected format");
    }
}

} // namespace Diligent

extern "C"
{
    void Diligent_CreateTextureFromFile(const Diligent::Char*            FilePath,
                                        const Diligent::TextureLoadInfo& TexLoadInfo,
                                        Diligent::IRenderDevice*         pDevice,
                                        Diligent::ITexture**             ppTexture)
    {
        Diligent::CreateTextureFromFile(FilePath, TexLoadInfo, pDevice, ppTexture);
    }
}
