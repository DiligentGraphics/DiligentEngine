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

#include <memory>

#include "ResourceReleaseQueue.hpp"
#include "DefaultRawMemoryAllocator.hpp"

#include "gtest/gtest.h"

using namespace Diligent;

namespace
{

TEST(GraphicsAccessories_ResourceReleaseQueue, GetFilterTypeLiteralName)
{
    struct ResourceA
    {
        int Data = 1;
    };
    struct ResourceB
    {
        float Data = 2;
    };
    struct ResourceC
    {
        float Data[2] = {3, 4};
    };

    {
        ResourceReleaseQueue<DynamicStaleResourceWrapper> Queue(DefaultRawMemoryAllocator::GetAllocator());

        std::unique_ptr<ResourceA> res0(new ResourceA);
        std::unique_ptr<ResourceB> res1(new ResourceB);

        Queue.SafeReleaseResource(std::move(res0), 0);
        Queue.SafeReleaseResource(std::move(res1), 0);

        std::unique_ptr<ResourceC> res2(new ResourceC);

        auto Wrapper2 = Queue.CreateWrapper(std::move(res2), 1);
        //auto WrapperX = Queue.CreateWrapper(res2, 1);// - error
        //auto WrapperY = Queue.CreateWrapper(static_cast<std::unique_ptr<ResourceC>&>(res2), 1);// - error
        Queue.SafeReleaseResource(std::move(Wrapper2), 0);

        Queue.DiscardStaleResources(0, 1);

        std::unique_ptr<ResourceA> res4(new ResourceA);
        Queue.DiscardResource(std::move(res4), 1);

        std::unique_ptr<ResourceB> res5(new ResourceB);

        auto Wrapper5 = Queue.CreateWrapper(std::move(res5), 1);
        Queue.DiscardResource(std::move(Wrapper5), 1);

        Queue.Purge(1);
    }

    {
        ResourceReleaseQueue<DynamicStaleResourceWrapper> Queue0(DefaultRawMemoryAllocator::GetAllocator());
        ResourceReleaseQueue<DynamicStaleResourceWrapper> Queue1(DefaultRawMemoryAllocator::GetAllocator());
        ResourceReleaseQueue<DynamicStaleResourceWrapper> Queue2(DefaultRawMemoryAllocator::GetAllocator());

        std::unique_ptr<ResourceA> res0(new ResourceA);
        std::unique_ptr<ResourceB> res1(new ResourceB);
        std::unique_ptr<ResourceC> res2(new ResourceC);

        auto Wrapper0 = ResourceReleaseQueue<DynamicStaleResourceWrapper>::CreateWrapper(std::move(res0), 3);
        auto Wrapper1 = ResourceReleaseQueue<DynamicStaleResourceWrapper>::CreateWrapper(std::move(res1), 3);
        auto Wrapper2 = ResourceReleaseQueue<DynamicStaleResourceWrapper>::CreateWrapper(std::move(res2), 1);

        Queue0.SafeReleaseResource(Wrapper0, 0);
        Queue0.SafeReleaseResource(Wrapper1, 0);
        Queue0.SafeReleaseResource(Wrapper2, 0);
        Wrapper2.GiveUpOwnership();

        Queue1.SafeReleaseResource(Wrapper0, 0);
        Queue1.SafeReleaseResource(Wrapper1, 0);

        Queue2.SafeReleaseResource(std::move(Wrapper0), 0);
        Queue2.SafeReleaseResource(Wrapper1, 0);
        Wrapper1.GiveUpOwnership();

        Queue0.DiscardStaleResources(0, 1);
        Queue1.DiscardStaleResources(0, 1);
        Queue2.DiscardStaleResources(0, 1);

        std::unique_ptr<ResourceC> res3(new ResourceC);

        auto Wrapper3 = ResourceReleaseQueue<DynamicStaleResourceWrapper>::CreateWrapper(std::move(res3), 2);
        Queue0.DiscardResource(Wrapper3, 1);
        Queue1.DiscardResource(std::move(Wrapper3), 1);

        std::unique_ptr<ResourceA> res4(new ResourceA);

        auto Wrapper4 = ResourceReleaseQueue<DynamicStaleResourceWrapper>::CreateWrapper(std::move(res4), 1);
        Queue2.DiscardResource(Wrapper4, 1);
        Wrapper4.GiveUpOwnership();

        Queue0.Purge(1);
        Queue1.Purge(1);
        Queue2.Purge(1);
    }
}

} // namespace
