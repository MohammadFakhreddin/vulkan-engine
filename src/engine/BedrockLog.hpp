#pragma once

#include "BedrockPlatforms.hpp"

#include <string>

#ifdef MFA_DEBUG
#define MFA_LOG_DEBUG(fmt_, ...)                    printf("\n-----------DEBUG------------\nFile: %s\nLine: %d\nFunction: %s\n" # fmt_, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__,)
#else
#define MFA_LOG_DEBUG(fmt_, ...)                    
#endif
#define MFA_LOG_INFO(fmt_, ...)                     printf("\n-----------INFO------------\nFile: %s\nLine: %d\nFunction: %s\n" #fmt_ "\n---------------------------\n", __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define MFA_LOG_WARN(fmt_, ...)                     printf("\n-----------WARN------------\nFile: %s\nLine: %d\nFunction: %s\n" #fmt_ "\n---------------------------\n", __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define MFA_LOG_ERROR(fmt_, ...)                    printf("\n-----------ERROR------------\nFile: %s\nLine: %d\nFunction: %s\n" #fmt_ "\n---------------------------\n", __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

namespace MFA::Log {} // MFA::Log
