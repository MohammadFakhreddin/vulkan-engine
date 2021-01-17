#pragma once

#include <cassert>
#include <stdexcept>
#include <cstring>

#include "engine/BedrockPlatforms.hpp"
#include "engine/BedrockLog.hpp"

#ifdef MFA_DEBUG
#define MFA_ASSERT(condition)           assert(condition); _assume(condition)
#define MFA_PTR_ASSERT(ptr)             MFA_ASSERT(MFA_PTR_VALID(ptr)); _assume(ptr != nullptr)
#define MFA_BLOB_ASSERT(blob)           MFA_PTR_ASSERT(blob.ptr); MFA_ASSERT(blob.len > 0)
#else
#define MFA_ASSERT(condition)           _assume(condition)
#define MFA_PTR_ASSERT(ptr)             _assume(ptr != nullptr)
#define MFA_BLOB_ASSERT(blob)
#endif

#define MFA_REQUIRE(condition)          if(!condition) {MFA_CRASH("Condition failed");}; _assume(condition)
#define MFA_NOT_IMPLEMENTED_YET(who)    throw std::runtime_error("Method not impelemented by " + std::string(who))

#define MFA_CRASH(message, ...)         MFA_LOG_ERROR(message, __VA_ARGS__); throw std::runtime_error(message)
#ifdef MFA_DEBUG
#define MFA_DEBUG_CRASH(message)        MFA_LOG_ERROR(message); throw std::runtime_error(message)
#else
#define MFA_DEBUG_CRASH(message)
#endif

namespace MFA::Assert {} // namespace MFA::Assert