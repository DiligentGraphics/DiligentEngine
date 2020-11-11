#include "volk.h"

#include "stdio.h"
#include "stdlib.h"

int main()
{
    VkResult r;
    uint32_t version;
    void* ptr;

    /* This won't compile if the appropriate Vulkan platform define isn't set. */
    ptr = 
#if defined(_WIN32)
    &vkCreateWin32SurfaceKHR;
#elif defined(__linux__) || defined(__unix__)
    &vkCreateXlibSurfaceKHR;
#elif defined(__APPLE__)
    &vkCreateMacOSSurfaceMVK;
#else
    /* Platform not recogized for testing. */
    NULL;
#endif

    /* Try to initialize volk. This might not work on CI builds, but the
     * above should have compiled at least. */
    r = volkInitialize();
    if (r != VK_SUCCESS) {
        printf("volkInitialize failed!\n");
        return -1;
    }

    version = volkGetInstanceVersion();
    printf("Vulkan version %d.%d.%d initialized.\n",
            VK_VERSION_MAJOR(version),
            VK_VERSION_MINOR(version),
            VK_VERSION_PATCH(version));

    return 0;
}

