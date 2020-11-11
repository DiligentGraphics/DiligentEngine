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

#pragma once

namespace Diligent
{

struct IRenderDevice;
struct IDeviceContext;
enum TEXTURE_FORMAT : uint16_t;

} // namespace Diligent

NK_API struct nk_diligent_context* nk_diligent_init(Diligent::IRenderDevice* device,
                                                    unsigned int             width,
                                                    unsigned int             height,
                                                    Diligent::TEXTURE_FORMAT BackBufferFmt,
                                                    Diligent::TEXTURE_FORMAT DepthBufferFmt,
                                                    unsigned int             max_vertex_buffer_size,
                                                    unsigned int             max_index_buffer_size);

NK_API struct nk_context* nk_diligent_get_nk_ctx(struct nk_diligent_context* nk_dlg_ctx);

NK_API void nk_diligent_font_stash_begin(struct nk_diligent_context* nk_dlg_ctx,
                                         struct nk_font_atlas**      atlas);

NK_API void nk_diligent_font_stash_end(struct nk_diligent_context* nk_dlg_ctx,
                                       Diligent::IDeviceContext*   device_ctx);

NK_API void nk_diligent_render(struct nk_diligent_context* nk_dlg_ctx,
                               Diligent::IDeviceContext*   device_ctx,
                               enum nk_anti_aliasing       AA);

NK_API void nk_diligent_resize(struct nk_diligent_context* nk_dlg_ctx,
                               Diligent::IDeviceContext*   device_ctx,
                               unsigned int                width,
                               unsigned int                height);

NK_API void nk_diligent_shutdown(struct nk_diligent_context* nk_dlg_ctx);
