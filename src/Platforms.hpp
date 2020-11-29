#ifndef PLATFORMS_NAMESPACE
#define PLATFORMS_NAMESPACE

#include <chrono>

namespace MFA {
namespace Platforms {
//
// Created by mohammad.fakhreddin on 1/3/20.
//

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    //define something for Windows (32-bit and 64-bit, this part is common)
#define __PLATFORM_WIN__
#define __DESKTOP__

#ifdef _WIN64
   //define something for Windows (64-bit only)
#else
   //define something for Windows (32-bit only)
#endif

#elif __APPLE__
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
    // iOS Simulator
    // iOS device
#define __MOBILE__
#ifndef __IOS__
#define __IOS__
#endif
#elif TARGET_OS_MAC
    // Other kinds of Mac OS
#define __DESKTOP__
#define __PLATFORM_MAC__
#else
#   error "Unknown Apple platform"
#endif
#elif __ANDROID__
#define __MOBILE__
#elif __linux__
    // linux
#define __PLATFORM_LINUX__
#if !defined(__DESKTOP__)
#define __DESKTOP__
#endif
#elif __unix__ // all unices not caught above
    // Unix
#define __PLATFORM_UNIX__
#elif defined(_POSIX_VERSION)
    // POSIX
#else
#   error "Unknown compiler"
#endif

#ifdef __PLATFORM_LINUX__
  using supportedClock = std::chrono::system_clock;
#else
  using supportedClock = std::chrono::steady_clock;
#endif

inline static constexpr char appName[] = "Vulkan engine";
  
enum class Platform {
  Windows,
  Mac,
  Iphone,
  Android,
  Unknown
};

#if defined(__PLATFORM_WIN__)
  #include <windows.h>
#elif defined(__PLATFORM_MAC__)
  #include <CoreGraphics/CGDisplayConfiguration.h>
#elif defined(__PLATFORM_LINUX__)
  // TODO
#endif

using ScreenSizeType = uint32_t;
struct ScreenInfo {
  ScreenSizeType screen_width = 0;
  ScreenSizeType screen_height = 0;
  bool valid = false;
};
static ScreenInfo ComputeScreenSize() {
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

}
}

#endif //!PLATFORMS_NAMESPACE