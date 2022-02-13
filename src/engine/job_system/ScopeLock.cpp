#include "ScopeLock.hpp"

#include "engine/BedrockAssert.hpp"

namespace MFA {

    ScopeLock::ScopeLock(std::atomic<bool> & lock)
        : mLock(lock)
    {
        bool expectedValue = false;
        bool const desiredValue = true;
        while (mLock.compare_exchange_strong(expectedValue, desiredValue) == false);
    }

    ScopeLock::~ScopeLock()
    {
        MFA_ASSERT(mLock == false);
        mLock = false;
    }
}
