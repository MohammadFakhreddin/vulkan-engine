#include "ScopeLock.hpp"

#include "engine/BedrockAssert.hpp"

namespace MFA {

    ScopeLock::ScopeLock(std::atomic<bool> & lock)
        : mLock(lock)
    {
        while (true)
        {
            bool expectedValue = false;
            bool const desiredValue = true;
            if (mLock.compare_exchange_strong(expectedValue, desiredValue) == true)
            {
                break;
            }
        }
        MFA_ASSERT(mLock == true);
    }

    ScopeLock::~ScopeLock()
    {
        MFA_ASSERT(mLock == true);
        mLock = false;
    }
}
