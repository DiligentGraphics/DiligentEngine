#pragma once

#include "PlatformDefinitions.h"

#if defined(PLATFORM_ANDROID)
#   include "Android/UnityGraphicsGLESAndroid_Impl.h"
#elif defined(PLATFORM_WIN32) || defined(PLATFORM_UNIVERSAL_WINDOWS) || defined(PLATFORM_LINUX)
#   include "UnityGraphicsGLCore_Impl.h"
#else
#   error Unknown Platform
#endif
