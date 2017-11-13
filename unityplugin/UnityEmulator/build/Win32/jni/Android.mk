
DEPENDENCY_PATH := $(call my-dir)
LOCAL_PATH := $(abspath $(DEPENDENCY_PATH))

include $(CLEAR_VARS)
# Project configuration
LOCAL_MODULE := UnityEmulator
LOCAL_CFLAGS := -std=c++11 -DENGINE_DLL
LOCAL_CPP_FEATURES := exceptions

LOCAL_STATIC_LIBRARIES := cpufeatures android_native_app_glue ndk_helper

# Include paths
PROJECT_ROOT := $(LOCAL_PATH)/../../..
ENGINE_ROOT := $(PROJECT_ROOT)/../..
CORE_ROOT := $(ENGINE_ROOT)/diligentcore
TOOLS_ROOT := $(ENGINE_ROOT)/diligenttools
# include directories for static libraries are declared in corresponding LOCAL_EXPORT_C_INCLUDES sections
LOCAL_C_INCLUDES += $(PROJECT_ROOT)/include
LOCAL_C_INCLUDES += $(PROJECT_ROOT)/src
LOCAL_C_INCLUDES += $(PROJECT_ROOT)/../GhostCubePlugin/PluginSource/src/Unity
LOCAL_C_INCLUDES += $(CORE_ROOT)/Common/include
LOCAL_C_INCLUDES += $(CORE_ROOT)/Common/interface
LOCAL_C_INCLUDES += $(CORE_ROOT)/Platforms/interface
LOCAL_C_INCLUDES += $(CORE_ROOT)/Graphics/GraphicsEngine/interface
LOCAL_C_INCLUDES += $(CORE_ROOT)/Graphics/GraphicsEngine/include
LOCAL_C_INCLUDES += $(CORE_ROOT)/Graphics/GraphicsEngineD3DBase/include
LOCAL_C_INCLUDES += $(CORE_ROOT)/Graphics/GraphicsEngineOpenGL/interface
LOCAL_C_INCLUDES += $(CORE_ROOT)/Graphics/HLSL2GLSLConverterLib/interface
LOCAL_C_INCLUDES += $(TOOLS_ROOT)/Graphics/GraphicsTools/include

# Source files
#VisualGDBAndroid: AutoUpdateSourcesInNextLine
LOCAL_SRC_FILES := ../../../src/Android/AndroidMainImpl.cpp ../../../src/Android/UnityGraphicsGLESAndroid_Impl.cpp ../../../src/DiligentGraphicsAdapterGL.cpp ../../../src/UnityGraphicsEmulator.cpp ../../../src/UnityGraphicsGLCoreES_Emulator.cpp ../../../src/Windows/WinMain.cpp

# Build instructions
#VisualGDBAndroid: VSExcludeListLocation
VISUALGDB_VS_EXCLUDED_FILES_Release := ../../../Src/Win32/WinMain.cpp
VISUALGDB_VS_EXCLUDED_FILES_Debug := ../../../src/Windows/WinMain.cpp
LOCAL_SRC_FILES := $(filter-out $(VISUALGDB_VS_EXCLUDED_FILES_$(VGDB_VSCONFIG)),$(LOCAL_SRC_FILES))

include $(BUILD_STATIC_LIBRARY)

$(call import-module,android/ndk_helper)
$(call import-module,android/native_app_glue)
$(call import-module,android/cpufeatures)
