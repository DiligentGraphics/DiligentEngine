#if PLATFORM_WIN32 || PLATFORM_UNIVERSAL_WINDOWS

#   ifdef _MSC_VER
#       include "tiffconf.vc.h"
#   else  // MinGW
#       include "tiffconf.linux.h"
#   endif

#elif PLATFORM_ANDROID || PLATFORM_LINUX || PLATFORM_MACOS || PLATFORM_IOS

#   include "tiffconf.linux.h"

#else

#   error Unsupported platform

#endif
