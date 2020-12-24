#pragma once

#include <cassert>

#define MFA_ASSERT(condition)  assert(condition)
#define MFA_NOT_IMPLEMENTED_YET() assert(false)

namespace MFA::Assert {} // namespace MFA::Assert