#if PLATFORM_WIN32 || PLATFORM_UNIVERSAL_WINDOWS

#   ifdef _MSC_VER
#       include "tif_config.vc.h"
#   else  // MinGW
#       include "tif_config.linux.h"
#   endif

#elif PLATFORM_ANDROID

#   include "tif_config.android.h"

#elif PLATFORM_LINUX || PLATFORM_MACOS || PLATFORM_IOS

#   include "tif_config.linux.h"

#else

#   error "Unknown platform"

#endif
