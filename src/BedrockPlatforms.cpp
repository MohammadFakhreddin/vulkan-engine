#include "BedrockPlatforms.hpp"

#if defined(__PLATFORM_WIN__)
  #include <windows.h>
#elif defined(__PLATFORM_MAC__)
  #include <CoreGraphics/CGDisplayConfiguration.h>
#elif defined(__PLATFORM_LINUX__)
  // TODO
#endif

namespace MFA::Platforms {

using ScreenSizeType = uint32_t;
ScreenInfo ComputeScreenSize() {
    ScreenInfo ret {};
    #if defined(__PLATFORM_WIN__)
        ScreenSizeType const real_screen_width = static_cast<int>(GetSystemMetrics(SM_CXSCREEN));
        ScreenSizeType const real_screen_height = static_cast<int>(GetSystemMetrics(SM_CYSCREEN));
        ret.screen_width = static_cast<ScreenSizeType>(real_screen_width);
        ret.screen_height = static_cast<ScreenSizeType>(real_screen_height);
        ret.valid = true;
    #elif defined(__PLATFORM_MAC__)
        auto main_display_id = CGMainDisplayID();	
        unsigned int real_screen_width = CGDisplayPixelsWide(main_display_id);
        unsigned int real_screen_height = CGDisplayPixelsHigh(main_display_id);
        ret.screen_width = static_cast<ScreenSizeType>(real_screen_width);
        ret.screen_height = static_cast<ScreenSizeType>(real_screen_height);
        ret.valid = true;
    #elif defined(__PLATFORM_LINUX__)
    // TODO
    #endif
    return ret;
}

} // namespace MFA::Platforms