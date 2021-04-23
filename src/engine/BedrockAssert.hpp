#pragma once

#include "engine/BedrockPlatforms.hpp"
#include "engine/BedrockLog.hpp"

#include <cassert>
#include <stdexcept>
#include <cstring>

#ifdef __PLATFORM_WIN__

#ifdef MFA_DEBUG
#define MFA_ASSERT(condition)           assert(condition); _assume(condition)
#else
#define MFA_ASSERT(condition)           _assume(condition)
#endif

#define MFA_REQUIRE(condition)          if(!(condition)) {MFA_CRASH("Condition failed");}; _assume(condition)

#define MFA_CRASH(message, ...)         MFA_LOG_ERROR(message, __VA_ARGS__); throw std::runtime_error(message)
#ifdef MFA_DEBUG
#define MFA_DEBUG_CRASH(message)        MFA_LOG_ERROR(message); throw std::runtime_error(message)
#else
#define MFA_DEBUG_CRASH(message)        throw std::runtime_error(message)
#endif

#else

#ifdef MFA_DEBUG
#define MFA_ASSERT(condition)           assert(condition);
#else
#define MFA_ASSERT(condition)           
#endif

#define MFA_REQUIRE(condition)          if(!(condition)) {MFA_CRASH("Condition failed");};

#define MFA_CRASH(message, ...)         throw std::runtime_error(message)// TODO MFA_LOG_ERROR(message, __VA_ARGS__); throw std::runtime_error(message)
#ifdef MFA_DEBUG
#define MFA_DEBUG_CRASH(message)        throw std::runtime_error(message)// TODO MFA_LOG_ERROR(message); throw std::runtime_error(message)
#else
#define MFA_DEBUG_CRASH(message)        throw std::runtime_error(message)
#endif

#endif

#define MFA_NOT_IMPLEMENTED_YET(who)    throw std::runtime_error("Method not implemented by " + std::string(who))



namespace MFA::Assert {} // namespace MFA::Assert