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

#include <vector>
#include <stdio.h>
#include <algorithm>

#include "MapHelper.hpp"
#include "GraphicsUtilities.h"
#include "TextureUtilities.h"
#include "TexturedCube.hpp"
#include "Actor.h"
#include "Component.h"

namespace Diligent
{

Actor::Actor()
{

}

Actor::Actor(const SampleInitInfo& InitInfo)
{
    Initialize(InitInfo);
}

void Actor::Initialize(const SampleInitInfo& InitInfo)
{
    SampleBase::Initialize(InitInfo);
}

void Actor::Update(double CurrTime, double ElapsedTime)
{
    SampleBase::Update(CurrTime, ElapsedTime);

    updateComponents(CurrTime, ElapsedTime);
    UpdateActor(CurrTime, ElapsedTime);
    computeWorldTransform();
}

void Actor::computeWorldTransform() 
{
    m_WorldMatrix = m_ContextInit;
    m_WorldMatrix *= m_WorldMatrix.Scale(getScale());
    m_WorldMatrix *= Quaternion::createFromQuaternion(getRotation());
    m_WorldMatrix *= m_WorldMatrix.Translation(getPosition());
}

void Actor::addComponent(Component* component)
{
    // Find the insertion point in the sorted vector
    // (The first element with a order higher than me)
    int  myOrder = component->getUpdateOrder();
    auto iter    = begin(components);
    for (; iter != end(components); ++iter)
    {
        if (myOrder < (*iter)->getUpdateOrder())
        {
            break;
        }
    }

    // Inserts element before position of iterator
    components.insert(iter, component);
}

void Actor::removeComponent(Component* component)
{
    auto iter = std::find(begin(components), end(components), component);
    if (iter != end(components))
    {
        components.erase(iter);
    }
}

void Actor::updateComponents(double CurrTime, double ElapsedTime)
{
    for (auto component : components)
    {
        component->update(CurrTime, ElapsedTime);
    }
}

} // namespace Diligent
