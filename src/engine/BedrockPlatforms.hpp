//
// Created by mohammad.fakhreddin on 1/3/20.
//
#pragma once

#include <chrono>

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

namespace MFA::Platforms {

inline static constexpr char appName [100] = "Vulkan engine";
  
enum class Platform {
    Windows,
    Mac,
    Iphone,
    Android,
    Unknown
};

#if defined(_DEBUG) && !defined(NDEBUG)
    #if !defined(MFA_DEBUG)
        #define MFA_DEBUG
    #endif
    #if defined(MFA_RELEASE)
        #undef MFA_RELEASE
    #endif
#elif !defined(_DEBUG) && defined(NDEBUG)
    #if defined(MFA_DEBUG)
        #undef MFA_DEBUG
    #endif
    #if !defined(MFA_RELEASE)
        #define MFA_RELEASE
    #endif
#else
    #ifndef NDEBUG
        #define NDEBUG
    #endif
    #ifdef _DEBUG
        #undef _DEBUG
    #endif
    #if !defined(MFA_RELEASE)
        #define MFA_RELEASE
    #endif
#endif

using ScreenSize = int32_t;
struct ScreenInfo {
  ScreenSize screenWidth = 0;
  ScreenSize screenHeight = 0;
  bool valid = false;
};
ScreenInfo ComputeScreenSize();
}; // namespace MFA::Platforms
