cmake_minimum_required (VERSION 3.6)

project(Diligent-Common CXX)

set(INCLUDE 
    include/pch.h
)

set(INTERFACE 
    interface/AdvancedMath.hpp
    interface/Align.hpp
    interface/BasicMath.hpp
    interface/BasicFileStream.hpp
    interface/DataBlobImpl.hpp
    interface/DefaultRawMemoryAllocator.hpp
    interface/FastRand.hpp
    interface/FileWrapper.hpp
    interface/FilteringTools.hpp
    interface/FixedBlockMemoryAllocator.hpp
    interface/HashUtils.hpp
    interface/LockHelper.hpp 
    interface/LinearAllocator.hpp 
    interface/MemoryFileStream.hpp 
    interface/ObjectBase.hpp
    interface/RefCntAutoPtr.hpp
    interface/RefCountedObjectImpl.hpp
    interface/STDAllocator.hpp
    interface/StringDataBlobImpl.hpp
    interface/StringTools.hpp
    interface/StringPool.hpp
    interface/ThreadSignal.hpp
    interface/Timer.hpp
    interface/UniqueIdentifier.hpp
    interface/ValidatedCast.hpp
)

set(SOURCE 
    src/BasicFileStream.cpp
    src/DataBlobImpl.cpp
    src/DefaultRawMemoryAllocator.cpp
    src/FixedBlockMemoryAllocator.cpp
    src/LockHelper.cpp
    src/MemoryFileStream.cpp
    src/Timer.cpp
)

add_library(Diligent-Common STATIC ${SOURCE} ${INCLUDE} ${INTERFACE})

target_include_directories(Diligent-Common 
PRIVATE
    include
PUBLIC 
    interface
)

target_link_libraries(Diligent-Common 
PRIVATE
    Diligent-BuildSettings
PUBLIC
    Diligent-TargetPlatform 
)
set_common_target_properties(Diligent-Common)

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})
source_group("interface" FILES ${INTERFACE})

set_target_properties(Diligent-Common PROPERTIES
    FOLDER DiligentCore
)

if(DILIGENT_INSTALL_CORE)
    install_core_lib(Diligent-Common)
endif()
