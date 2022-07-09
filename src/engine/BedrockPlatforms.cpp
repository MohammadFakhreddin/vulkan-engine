#include "BedrockPlatforms.hpp"

#if defined(__PLATFORM_WIN__)
  #include <windows.h>
#elif defined(__PLATFORM_MAC__)
  #include <CoreGraphics/CGDisplayConfiguration.h>
#elif defined(__PLATFORM_LINUX__)
  #include <X11/Xlib.h>
#endif

namespace MFA::Platforms {
// TODO: We can use device capabilities instead
ScreenInfo ComputeScreenSize() {
    ScreenInfo ret {};
    #if defined(__PLATFORM_WIN__)
        auto const real_screen_width = static_cast<int>(GetSystemMetrics(SM_CXSCREEN));
        auto const real_screen_height = static_cast<int>(GetSystemMetrics(SM_CYSCREEN));
        ret.screenWidth = static_cast<ScreenSize>(real_screen_width);
        ret.screenHeight = static_cast<ScreenSize>(real_screen_height);
        ret.valid = true;
    #elif defined(__PLATFORM_MAC__)
        auto mainDisplayId = CGMainDisplayID();
        unsigned int realScreenWidth = CGDisplayPixelsWide(mainDisplayId);
        unsigned int realScreenHeight = CGDisplayPixelsHigh(mainDisplayId);
        ret.screenWidth = static_cast<ScreenSize>(realScreenWidth);
        ret.screenHeight = static_cast<ScreenSize>(realScreenHeight);
        ret.valid = true;
    #elif defined(__PLATFORM_LINUX__)
        Display* display = XOpenDisplay(NULL);
        Screen*  screen = DefaultScreenOfDisplay(display);
        ret.screenWidth = (int)screen->width;
        ret.screenHeight = (int)screen->height;
        ret.valid = true;
    #endif
    return ret;
}

} // namespace MFA::Platforms
