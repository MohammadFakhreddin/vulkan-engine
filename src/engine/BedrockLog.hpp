#pragma once

#include "BedrockPlatforms.hpp"

#ifdef __ANDROID__
#include <android/log.h>
#include <android_native_app_glue.h>
#endif

#include <string>

#ifdef MFA_DEBUG
    #if defined(__DESKTOP__) || defined(__IOS__)
        #define MFA_LOG_DEBUG(fmt_, ...)                printf("\n-----------DEBUG------------\nFile: %s\nLine: %d\nFunction: %s\n" # fmt_, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
    #elif defined(__ANDROID__)
        #define MFA_LOG_DEBUG(fmt_, ...)                __android_log_print(ANDROID_LOG_DEBUG, "MFA", "\n-----------DEBUG------------\nFile: %s\nLine: %d\nFunction: %s\n" # fmt_, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
    #else
        #error Os is not supported
    #endif
#else
#define MFA_LOG_DEBUG(fmt_)                         
#endif

#if defined(__DESKTOP__) || defined(__IOS__)
    #define MFA_LOG_INFO(fmt_, ...)                     printf("\n-----------INFO------------\nFile: %s\nLine: %d\nFunction: %s\n" #fmt_ "\n---------------------------\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
    #define MFA_LOG_WARN(fmt_, ...)                     printf("\n-----------WARN------------\nFile: %s\nLine: %d\nFunction: %s\n" #fmt_ "\n---------------------------\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
    #define MFA_LOG_ERROR(fmt_, ...)                    printf("\n-----------ERROR------------\nFile: %s\nLine: %d\nFunction: %s\n" #fmt_ "\n---------------------------\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#elif defined(__ANDROID__)
    #define MFA_LOG_INFO(fmt_, ...)                     __android_log_print(ANDROID_LOG_INFO, "MFA", "\n-----------INFO------------\nFile: %s\nLine: %d\nFunction: %s\n" #fmt_ "\n---------------------------\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
    #define MFA_LOG_WARN(fmt_, ...)                     __android_log_print(ANDROID_LOG_WARN, "MFA", "\n-----------WARN------------\nFile: %s\nLine: %d\nFunction: %s\n" #fmt_ "\n---------------------------\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
    #define MFA_LOG_ERROR(fmt_, ...)                    __android_log_print(ANDROID_LOG_ERROR, "MFA", "\n-----------ERROR------------\nFile: %s\nLine: %d\nFunction: %s\n" #fmt_ "\n---------------------------\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
    #error Os is not supported
#endif

namespace MFA::Log {} // MFA::Log
