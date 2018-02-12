#pragma once

#include "PlatformDefinitions.h"

#if PLATFORM_ANDROID
#   include "Android/UnityGraphicsGLESAndroid_Impl.h"
#elif PLATFORM_WIN32 || PLATFORM_UNIVERSAL_WINDOWS || PLATFORM_LINUX || PLATFORM_MACOS
#   include "UnityGraphicsGLCore_Impl.h"
#elif PLATFORM_IOS
#   include "IOS/UnityGraphicsGLES_IOS_Impl.h"
#else
#   error Unknown Platform
#endif
