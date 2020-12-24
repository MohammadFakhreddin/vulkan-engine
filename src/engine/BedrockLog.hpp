#pragma once

#include "BedrockPlatforms.hpp"

#include <string>

#ifdef MFA_DEBUG
#define MFA_LOG_DEBUG(fmt_, ...)                    printf("-----------DEBUG------------\nFile: %s\nLine: %d\nFunction: %s\n" # fmt_, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__,)
#else
#define MFA_LOG_DEBUG(fmt_, ...)                    
#endif
#define MFA_LOG_INFO(fmt_, ...)                     printf("-----------INFO------------\nFile: %s\nLine: %d\nFunction: %s\n" # fmt_, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define MFA_LOG_WARN(fmt_, ...)                     printf("-----------WARN------------\nFile: %s\nLine: %d\nFunction: %s\n" # fmt_, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define MFA_LOG_ERROR(fmt_, ...)                    printf("-----------ERROR------------\nFile: %s\nLine: %d\nFunction: %s\n" # fmt_, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

// TODO Actual PrintFmt also remove fucking VA_OPT

namespace MFA::Log {} // MFA::Log
