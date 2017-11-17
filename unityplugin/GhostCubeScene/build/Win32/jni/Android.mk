
DEPENDENCY_PATH := $(call my-dir)
LOCAL_PATH := $(abspath $(DEPENDENCY_PATH))

include $(CLEAR_VARS)
# Project configuration
LOCAL_MODULE := GhostCubeSceneEmulator
LOCAL_CFLAGS := -std=c++11
LOCAL_CPP_FEATURES := exceptions
LOCAL_LDLIBS := -lGLESv3 -lEGL -llog -landroid

#                                   !!!!! VERY IMPORTANT !!!!!
# Not too smart gcc linker links libraries in the order they are declared. From each library it only exports the 
# methods/functions required to resolve CURRENTLY OUTSTANDING dependencies and ignores the rest. 
# If a subsequent library then uses methods/functions that were not originally required by the objects, you will 
# have missing dependencies.
LOCAL_STATIC_LIBRARIES := UnityEmulator-prebuilt GraphicsEngine-prebuilt GraphicsTools-prebuilt cpufeatures android_native_app_glue NdkHelper-prebuilt
# These libraries depend on each other
LOCAL_WHOLE_STATIC_LIBRARIES := AndroidPlatform-prebuilt BasicPlatform-prebuilt Common-prebuilt
LOCAL_SHARED_LIBRARIES := GhostCubePlugin-prebuilt GraphicsEngineOpenGL-prebuilt

# Include paths
PROJECT_ROOT := $(LOCAL_PATH)/../../..
ENGINE_ROOT := $(PROJECT_ROOT)/../..
CORE_ROOT := $(ENGINE_ROOT)/diligentcore
TOOLS_ROOT := $(ENGINE_ROOT)/diligenttools
SAMPLES_ROOT := $(ENGINE_ROOT)/diligentsamples

LOCAL_C_INCLUDES += $(PROJECT_ROOT)/../GhostCubePlugin/PluginSource/src/Unity
# include directories for static libraries are declared in corresponding LOCAL_EXPORT_C_INCLUDES sections


# Source files
#VisualGDBAndroid: AutoUpdateSourcesInNextLine
LOCAL_SRC_FILES := ../../../src/Android/AndroidMain.cpp ../../../src/GhostCubeScene.cpp

include $(BUILD_SHARED_LIBRARY)


# Declare pre-built static libraries

include $(CLEAR_VARS)
# Declare pre-built Common library
LOCAL_MODULE := UnityEmulator-prebuilt
LOCAL_SRC_FILES := $(PROJECT_ROOT)/../UnityEmulator/build/Win32/obj/local/$(TARGET_ARCH_ABI)/libUnityEmulator.a

# The LOCAL_EXPORT_C_INCLUDES definition ensures that any module that depends on the 
# prebuilt one will have its LOCAL_C_INCLUDES automatically prepended with the path to the 
# prebuilt's include directory, and will thus be able to find headers inside that.
LOCAL_EXPORT_C_INCLUDES := $(PROJECT_ROOT)/../UnityEmulator/include

include $(PREBUILT_STATIC_LIBRARY)


# Declare pre-built static libraries

include $(CLEAR_VARS)
# Declare pre-built Common library
LOCAL_MODULE := Common-prebuilt
LOCAL_SRC_FILES := $(CORE_ROOT)/Common/build/Win32/obj/local/$(TARGET_ARCH_ABI)/libCommon.a

# The LOCAL_EXPORT_C_INCLUDES definition ensures that any module that depends on the 
# prebuilt one will have its LOCAL_C_INCLUDES automatically prepended with the path to the 
# prebuilt's include directory, and will thus be able to find headers inside that.
LOCAL_EXPORT_C_INCLUDES := $(CORE_ROOT)/Common/include $(CORE_ROOT)/Common/interface $(CORE_ROOT)/Platforms/interface

include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
# Declare pre-built AndroidPlatform library
LOCAL_MODULE := AndroidPlatform-prebuilt
LOCAL_SRC_FILES := $(CORE_ROOT)/Platforms/Android/build/Windows/obj/local/$(TARGET_ARCH_ABI)/libAndroidPlatform.a
LOCAL_EXPORT_C_INCLUDES := $(CORE_ROOT)/Platforms/interface
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
# Declare pre-built BasicPlatform library
LOCAL_MODULE := BasicPlatform-prebuilt
LOCAL_SRC_FILES := $(CORE_ROOT)/Platforms/Basic/build/Win32/obj/local/$(TARGET_ARCH_ABI)/libBasicPlatform.a
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
# Declare pre-built GraphicsEngine library
LOCAL_MODULE := GraphicsEngine-prebuilt
LOCAL_SRC_FILES := $(CORE_ROOT)/Graphics/GraphicsEngine/build/Win32/obj/local/$(TARGET_ARCH_ABI)/libGraphicsEngine.a
LOCAL_EXPORT_C_INCLUDES := $(CORE_ROOT)/Graphics/GraphicsEngine/interface
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
# Declare pre-built GraphicsEngineOpenGL library
LOCAL_MODULE := GraphicsEngineOpenGL-prebuilt
LOCAL_SRC_FILES := $(CORE_ROOT)/Graphics/GraphicsEngineOpenGL/build/Win32/libs/$(TARGET_ARCH_ABI)/libGraphicsEngineOpenGL.so
LOCAL_EXPORT_C_INCLUDES := $(CORE_ROOT)/Graphics/GraphicsEngineOpenGL/interface
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
# Declare pre-built Graphics Tools library
LOCAL_MODULE := GraphicsTools-prebuilt
LOCAL_SRC_FILES := $(CORE_ROOT)/Graphics/GraphicsTools/build/Win32/obj/local/$(TARGET_ARCH_ABI)/libGraphicsTools.a
LOCAL_EXPORT_C_INCLUDES := $(CORE_ROOT)/Graphics/GraphicsTools/include
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
# Declare pre-built GhostCubePlugin library
LOCAL_MODULE := GhostCubePlugin-prebuilt
LOCAL_SRC_FILES := $(PROJECT_ROOT)/../GhostCubePlugin/PluginSource/build/Win32/libs/$(TARGET_ARCH_ABI)/libGhostCubePlugin.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := NdkHelper-prebuilt
LOCAL_SRC_FILES := $(CORE_ROOT)/External/Android/ndk_helper/build/obj/local/$(TARGET_ARCH_ABI)/libNdkHelper.a
LOCAL_EXPORT_C_INCLUDES := $(CORE_ROOT)/External/Android/ndk_helper/include
include $(PREBUILT_STATIC_LIBRARY)


$(call import-module,android/native_app_glue)
$(call import-module,android/cpufeatures)

