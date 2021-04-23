#pragma once

#include "BedrockPlatforms.hpp"

#include <string>

#ifndef __PLATFORM_MAC__

#ifdef MFA_DEBUG
#define MFA_LOG_DEBUG(fmt_, ...)                    printf("\n-----------DEBUG------------\nFile: %s\nLine: %d\nFunction: %s\n" # fmt_, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__,)
#else
#define MFA_LOG_DEBUG(fmt_, ...)                    
#endif
#define MFA_LOG_INFO(fmt_, ...)                     printf("\n-----------INFO------------\nFile: %s\nLine: %d\nFunction: %s\n" #fmt_ "\n---------------------------\n", __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define MFA_LOG_WARN(fmt_, ...)                     printf("\n-----------WARN------------\nFile: %s\nLine: %d\nFunction: %s\n" #fmt_ "\n---------------------------\n", __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define MFA_LOG_ERROR(fmt_, ...)                    printf("\n-----------ERROR------------\nFile: %s\nLine: %d\nFunction: %s\n" #fmt_ "\n---------------------------\n", __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#else

#ifdef MFA_DEBUG
#define MFA_LOG_DEBUG(fmt_, ...)                    // TODO printf("\n-----------DEBUG------------\nFile: %s\nLine: %d\nFunction: %s\n" # fmt_, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__,)
#else
#define MFA_LOG_DEBUG(fmt_, ...)                    
#endif
#define MFA_LOG_INFO(fmt_, ...)                     // TODO printf("\n-----------INFO------------\nFile: %s\nLine: %d\nFunction: %s\n" #fmt_ "\n---------------------------\n", __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define MFA_LOG_WARN(fmt_, ...)                     // TODO printf("\n-----------WARN------------\nFile: %s\nLine: %d\nFunction: %s\n" #fmt_ "\n---------------------------\n", __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define MFA_LOG_ERROR(fmt_, ...)                    // TODOprintf("\n-----------ERROR------------\nFile: %s\nLine: %d\nFunction: %s\n" #fmt_ "\n---------------------------\n", __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)


#endif

namespace MFA::Log {} // MFA::Log
