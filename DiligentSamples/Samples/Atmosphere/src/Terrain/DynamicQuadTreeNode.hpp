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

// This file is derived from the open source project provided by Intel Corportaion that
// requires the following notice to be kept:
//--------------------------------------------------------------------------------------
// Copyright 2013 Intel Corporation
// All Rights Reserved
//
// Permission is granted to use, copy, distribute and prepare derivative works of this
// software for any purpose and without fee, provided, that the above copyright notice
// and this statement appear in all copies.  Intel makes no representations about the
// suitability of this software for any purpose.  THIS SOFTWARE IS PROVIDED "AS IS."
// INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, AND ALL LIABILITY,
// INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE,
// INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not
// assume any responsibility for any errors which may appear in this software nor any
// responsibility to update it.
//--------------------------------------------------------------------------------------

#pragma once

#include <memory>
#include <cassert>

namespace Diligent
{

// Structure describing quad tree node location
struct QuadTreeNodeLocation
{
    // Position in a tree
    int horzOrder;
    int vertOrder;
    int level;

    QuadTreeNodeLocation(int h, int v, int l) :
        horzOrder(h), vertOrder(v), level(l)
    {
        VERIFY_EXPR(h < (1 << l));
        VERIFY_EXPR(v < (1 << l));
    }
    QuadTreeNodeLocation() :
        horzOrder(0), vertOrder(0), level(0)
    {}

    // Gets location of a child
    inline friend QuadTreeNodeLocation GetChildLocation(const QuadTreeNodeLocation& parent,
                                                        unsigned int                siblingOrder)
    {
        VERIFY_EXPR(siblingOrder >= 0 && siblingOrder < 4);
        return QuadTreeNodeLocation(parent.horzOrder * 2 + (siblingOrder & 1),
                                    parent.vertOrder * 2 + (siblingOrder >> 1),
                                    parent.level + 1);
    }

    // Gets location of a parent
    inline friend QuadTreeNodeLocation GetParentLocation(const QuadTreeNodeLocation& node)
    {
        assert(node.level > 0);
        return QuadTreeNodeLocation(node.horzOrder / 2, node.vertOrder / 2, node.level - 1);
    }
};

// Base class for iterators traversing the quad tree
class HierarchyIteratorBase
{
public:
    operator const QuadTreeNodeLocation&() const { return m_current; }
    int Level() const { return m_current.level; }
    int Horz() const { return m_current.horzOrder; }
    int Vert() const { return m_current.vertOrder; }

protected:
    QuadTreeNodeLocation m_current;
    int                  m_currentLevelSize;
};

// Iterator for recursively traversing the quad tree starting from the root up to the specified level
class HierarchyIterator : public HierarchyIteratorBase
{
public:
    HierarchyIterator(int nLevels) :
        m_nLevels(nLevels)
    {
        m_currentLevelSize = 1;
    }
    bool IsValid() const { return m_current.level < m_nLevels; }
    void Next()
    {
        if (++m_current.horzOrder == m_currentLevelSize)
        {
            m_current.horzOrder = 0;
            if (++m_current.vertOrder == m_currentLevelSize)
            {
                m_current.vertOrder = 0;
                m_currentLevelSize  = 1 << ++m_current.level;
            }
        }
    }

private:
    int m_nLevels;
};

// Iterator for recursively traversing the quad tree starting from the specified level up to the root
class HierarchyReverseIterator : public HierarchyIteratorBase
{
public:
    HierarchyReverseIterator(int nLevels)
    {
        m_current.level    = nLevels - 1;
        m_currentLevelSize = 1 << m_current.level;
    }
    bool IsValid() const { return m_current.level >= 0; }
    void Next()
    {
        if (++m_current.horzOrder == m_currentLevelSize)
        {
            m_current.horzOrder = 0;
            if (++m_current.vertOrder == m_currentLevelSize)
            {
                m_current.vertOrder = 0;
                m_currentLevelSize  = 1 << --m_current.level;
            }
        }
    }
};

// Template class for the node of a dynamic quad tree
template <typename NodeDataType>
class DynamicQuadTreeNode
{
public:
    DynamicQuadTreeNode() :
        m_pAncestor(NULL)
    {
    }

    NodeDataType&       GetData() { return m_Data; }
    const NodeDataType& GetData() const { return m_Data; }

    DynamicQuadTreeNode* GetAncestor() const { return m_pAncestor; }

    void GetDescendants(const DynamicQuadTreeNode*& LBDescendant,
                        const DynamicQuadTreeNode*& RBDescendant,
                        const DynamicQuadTreeNode*& LTDescendant,
                        const DynamicQuadTreeNode*& RTDescendant) const
    {
        LBDescendant = m_pLBDescendant.get();
        RBDescendant = m_pRBDescendant.get();
        LTDescendant = m_pLTDescendant.get();
        RTDescendant = m_pRTDescendant.get();
    }

    void GetDescendants(DynamicQuadTreeNode*& LBDescendant,
                        DynamicQuadTreeNode*& RBDescendant,
                        DynamicQuadTreeNode*& LTDescendant,
                        DynamicQuadTreeNode*& RTDescendant)
    {
        LBDescendant = m_pLBDescendant.get();
        RBDescendant = m_pRBDescendant.get();
        LTDescendant = m_pLTDescendant.get();
        RTDescendant = m_pRTDescendant.get();
    }

    typedef std::unique_ptr<DynamicQuadTreeNode<NodeDataType>> AutoPtrType;
    // Attahes specified descendants to the tree
    void CreateDescendants(AutoPtrType pLBDescendant,
                           AutoPtrType pRBDescendant,
                           AutoPtrType pLTDescendant,
                           AutoPtrType pRTDescendant);
    // Creates descendants UNATTACHED to the tree
    void CreateFloatingDescendants(AutoPtrType& pLBDescendant,
                                   AutoPtrType& pRBDescendant,
                                   AutoPtrType& pLTDescendant,
                                   AutoPtrType& pRTDescendant);
    // Destroys ALL descendants for the node
    void DestroyDescendants();

    const QuadTreeNodeLocation& GetPos() const { return m_pos; }

    void SetPos(const QuadTreeNodeLocation& pos) { m_pos = pos; }

private:
    DynamicQuadTreeNode(DynamicQuadTreeNode* pAncestor, int iSiblingOrder) :
        m_pAncestor(pAncestor),
        m_pos(GetChildLocation(pAncestor->m_pos, iSiblingOrder))
    {
    }

    NodeDataType m_Data;

    std::unique_ptr<DynamicQuadTreeNode> m_pLBDescendant;
    std::unique_ptr<DynamicQuadTreeNode> m_pRBDescendant;
    std::unique_ptr<DynamicQuadTreeNode> m_pLTDescendant;
    std::unique_ptr<DynamicQuadTreeNode> m_pRTDescendant;
    DynamicQuadTreeNode*                 m_pAncestor;

    QuadTreeNodeLocation m_pos;
};

template <typename NodeDataType>
void DynamicQuadTreeNode<NodeDataType>::CreateFloatingDescendants(AutoPtrType& pLBDescendant,
                                                                  AutoPtrType& pRBDescendant,
                                                                  AutoPtrType& pLTDescendant,
                                                                  AutoPtrType& pRTDescendant)
{
    pLBDescendant.reset(new DynamicQuadTreeNode<NodeDataType>(this, 0));
    pRBDescendant.reset(new DynamicQuadTreeNode<NodeDataType>(this, 1));
    pLTDescendant.reset(new DynamicQuadTreeNode<NodeDataType>(this, 2));
    pRTDescendant.reset(new DynamicQuadTreeNode<NodeDataType>(this, 3));
}

template <typename NodeDataType>
void DynamicQuadTreeNode<NodeDataType>::CreateDescendants(AutoPtrType pLBDescendant,
                                                          AutoPtrType pRBDescendant,
                                                          AutoPtrType pLTDescendant,
                                                          AutoPtrType pRTDescendant)
{
    assert(!m_pLBDescendant.get());
    assert(!m_pRBDescendant.get());
    assert(!m_pLTDescendant.get());
    assert(!m_pRTDescendant.get());

    m_pLBDescendant = pLBDescendant;
    m_pRBDescendant = pRBDescendant;
    m_pLTDescendant = pLTDescendant;
    m_pRTDescendant = pRTDescendant;
}

template <typename NodeDataType>
void DynamicQuadTreeNode<NodeDataType>::DestroyDescendants()
{
    if (m_pLBDescendant.get()) m_pLBDescendant->DestroyDescendants();
    if (m_pRBDescendant.get()) m_pRBDescendant->DestroyDescendants();
    if (m_pLTDescendant.get()) m_pLTDescendant->DestroyDescendants();
    if (m_pRTDescendant.get()) m_pRTDescendant->DestroyDescendants();

    m_pLBDescendant.reset();
    m_pRBDescendant.reset();
    m_pLTDescendant.reset();
    m_pRTDescendant.reset();
}

} // namespace Diligent
