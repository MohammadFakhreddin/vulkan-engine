#pragma once

#include "engine/BedrockPlatforms.hpp"
#include "engine/BedrockLog.hpp"

#include <stdexcept>
#include <cassert>

#ifdef __PLATFORM_WIN__

#ifdef MFA_DEBUG
#define MFA_ASSERT(condition)           assert(condition); _assume(condition)
#else
#define MFA_ASSERT(condition)           _assume(condition)
#endif

#define MFA_REQUIRE(condition)          if(!(condition)) {MFA_CRASH("Condition failed");}; _assume(condition)

#define MFA_CRASH(message, ...)         MFA_LOG_ERROR(message, __VA_ARGS__); throw std::runtime_error(message)


#else

#ifdef MFA_DEBUG
#define MFA_ASSERT(condition)           assert(condition);
#else
#define MFA_ASSERT(condition)           
#endif

#define MFA_REQUIRE(condition)          if(!(condition)) {MFA_CRASH("Condition failed");};

#define MFA_CRASH(message, ...)         throw std::runtime_error(message)// TODO MFA_LOG_ERROR(message, __VA_ARGS__); throw std::runtime_error(message)


#endif

#define MFA_NOT_IMPLEMENTED_YET(who)    MFA_LOG_ERROR(who); throw std::runtime_error("Method not implemented by " + std::string(who))

bool constexpr MFA_VERIFY(bool const condition) {
    if (condition) {
        return true;
    }
#ifdef MFA_DEBUG
    throw std::runtime_error("Condition failed");
#endif
    return false;
}

#if defined(__DESKTOP__) || defined(__IOS__)
#define MFA_VK_VALID(vkVariable) vkVariable != nullptr
#define MFA_VK_INVALID(vkVariable) vkVariable == nullptr
#define MFA_VK_VALID_ASSERT(vkVariable) MFA_ASSERT(MFA_VK_VALID(vkVariable))
#define MFA_VK_INVALID_ASSERT(vkVariable) MFA_ASSERT(MFA_VK_INVALID(vkVariable))
#define MFA_VK_MAKE_NULL(vkVariable) vkVariable = nullptr
#elif defined(__ANDROID__)
#define MFA_VK_VALID(vkVariable) vkVariable != 0
#define MFA_VK_INVALID(vkVariable) vkVariable == 0
#define MFA_VK_VALID_ASSERT(vkVariable) MFA_ASSERT(MFA_VK_VALID(vkVariable))
#define MFA_VK_INVALID_ASSERT(vkVariable) MFA_ASSERT(MFA_VK_INVALID(vkVariable))
#define MFA_VK_MAKE_NULL(vkVariable) vkVariable = 0
#else
#error Unhandled platform
#endif

namespace MFA::Assert {} // namespace MFA::Assert