cmake_minimum_required (VERSION 3.6)

project(Diligent-Primitives CXX)

set(SOURCE
    src/DebugOutput.cpp
    src/test.cpp
)

set(INTERFACE
    interface/BasicTypes.h
    interface/CommonDefinitions.h
    interface/DataBlob.h
    interface/DebugOutput.h
    interface/DefineGlobalFuncHelperMacros.h
    interface/DefineInterfaceHelperMacros.h
    interface/FlagEnum.h
    interface/Errors.hpp
    interface/FileStream.h
    interface/FormatString.hpp
    interface/InterfaceID.h
    interface/MemoryAllocator.h
    interface/Object.h
    interface/ReferenceCounters.h
    interface/UndefGlobalFuncHelperMacros.h
    interface/UndefInterfaceHelperMacros.h
)

# This should be an interface library. However, CMake does not show
# interface libraries in an IDE, so we have to create a static library instead
add_library(Diligent-Primitives STATIC ${INTERFACE} ${SOURCE})
set_common_target_properties(Diligent-Primitives)

target_link_libraries(Diligent-Primitives 
PRIVATE
    Diligent-BuildSettings
)

target_include_directories(Diligent-Primitives
PUBLIC
    interface
)

source_group("interface" FILES ${INTERFACE})
source_group("src" FILES ${SOURCE})

set_target_properties(Diligent-Primitives PROPERTIES
    FOLDER DiligentCore
)

if(DILIGENT_INSTALL_CORE)
    install_core_lib(Diligent-Primitives)
endif()