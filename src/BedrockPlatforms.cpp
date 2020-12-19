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
struct ScreenInfo {
  ScreenSizeType screen_width = 0;
  ScreenSizeType screen_height = 0;
  bool valid = false;
};
ScreenInfo ComputeScreenSize() {
    ScreenInfo info {};
    #if defined(__PLATFORM_WIN__)
        ScreenSizeType const real_screen_width = static_cast<int>(GetSystemMetrics(SM_CXSCREEN));
        ScreenSizeType const real_screen_height = static_cast<int>(GetSystemMetrics(SM_CYSCREEN));
        info.screen_width = static_cast<ScreenSizeType>(real_screen_width);
        info.screen_height = static_cast<ScreenSizeType>(real_screen_height);
        info.valid = true;
    #elif defined(__PLATFORM_MAC__)
        auto main_display_id = CGMainDisplayID();	
        unsigned int real_screen_width = CGDisplayPixelsWide(main_display_id);
        unsigned int real_screen_height = CGDisplayPixelsHigh(main_display_id);
        info.screen_width = static_cast<ScreenSizeType>(real_screen_width);
        info.screen_height = static_cast<ScreenSizeType>(real_screen_height);
        info.valid = true;
    #elif defined(__PLATFORM_LINUX__)
    // TODO
    #endif
    return info;
}

} // namespace MFA::Platforms