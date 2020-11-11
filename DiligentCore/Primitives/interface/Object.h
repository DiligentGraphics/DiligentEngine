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

/// \file
/// Defines Diligent::IObject interface

#include "InterfaceID.h"
#include "ReferenceCounters.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

#if DILIGENT_CPP_INTERFACE

/// Base interface for all dynamic objects in the engine
struct IObject
{
    /// Queries the specific interface.

    /// \param [in] IID - Unique identifier of the requested interface.
    /// \param [out] ppInterface - Memory address where the pointer to the requested interface will be written.
    ///                            If the interface is not supported, null pointer will be returned.
    /// \remark The method increments the number of strong references by 1. The interface must be
    ///         released by a call to Release() method when it is no longer needed.
    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) = 0;


    /// Increments the number of strong references by 1.

    /// \remark This method is equivalent to GetReferenceCounters()->AddStrongRef().\n
    ///         The method is thread-safe and does not require explicit synchronization.
    /// \return The number of strong references after incrementing the counter.
    /// \note   In a multithreaded environment, the returned number may not be reliable
    ///         as other threads may simultaneously change the actual value of the counter.
    virtual ReferenceCounterValueType DILIGENT_CALL_TYPE AddRef() = 0;


    /// Decrements the number of strong references by 1 and destroys the object when the
    /// counter reaches zero.

    /// \remark This method is equivalent to GetReferenceCounters()->ReleaseStrongRef().\n
    ///         The method is thread-safe and does not require explicit synchronization.
    /// \return The number of strong references after decrementing the counter.
    /// \note   In a multithreaded environment, the returned number may not be reliable
    ///         as other threads may simultaneously change the actual value of the counter.
    ///         The only reliable value is 0 as the object is destroyed when the last
    ///         strong reference is released.
    virtual ReferenceCounterValueType DILIGENT_CALL_TYPE Release() = 0;


    /// Returns the pointer to IReferenceCounters interface of the associated
    /// reference counters object. The method does *NOT* increment
    /// the number of strong references to the returned object.
    virtual IReferenceCounters* DILIGENT_CALL_TYPE GetReferenceCounters() const = 0;
};

#else

struct IObject;

// clang-format off

typedef struct IObjectMethods
{
    void                       (*QueryInterface)      (struct IObject*, const struct INTERFACE_ID* IID, struct IObject** ppInterface);
    ReferenceCounterValueType  (*AddRef)              (struct IObject*);
    ReferenceCounterValueType  (*Release)             (struct IObject*);
    struct IReferenceCounters* (*GetReferenceCounters)(struct IObject*);
} IObjectMethods;

#define IObjectInclusiveMethods IObjectMethods Object

typedef struct IObjectVtbl
{
    IObjectInclusiveMethods;
} IObjectVtbl;

// clang-format on

typedef struct IObject
{
    struct IObjectVtbl* pVtbl;
} IObject;

// clang-format off

#    define IObject_QueryInterface(This, ...) CALL_IFACE_METHOD(Object, QueryInterface, This, __VA_ARGS__)
#    define IObject_AddRef(This)              CALL_IFACE_METHOD(Object, AddRef,         This)
#    define IObject_Release(This)             CALL_IFACE_METHOD(Object, Release,        This)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
