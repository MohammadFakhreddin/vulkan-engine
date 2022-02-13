#pragma once

#include "engine/BedrockAssert.hpp"
#include "engine/job_system/ScopeLock.hpp"

#include <queue>

namespace MFA {

template <typename T>
class ThreadSafeQueue {
public:

    bool TryToPush(T newData) {
        bool expectedValue = false;
        bool const desiredValue = true;
        if (mIsLocked.compare_exchange_strong(expectedValue, desiredValue) == false) {
            return false;
        }

        MFA_ASSERT(mIsLocked == true);
        mData.push(newData);
        mIsLocked = false;
        return true;
    }

    // Returns front item
    bool TryToPop(T & outData) {
        bool expectedValue = false;
        bool const desiredValue = true;
        if (mIsLocked.compare_exchange_strong(expectedValue, desiredValue) == false) {
            return false;
        }

        MFA_ASSERT(mIsLocked == true);
        bool success = false;
        if (mData.empty() == false) {
            outData = mData.front();
            mData.pop();
            success = true;
        }
        mIsLocked = false;
        return success;
    }

    [[nodiscard]]
    bool IsEmpty() {
        ScopeLock scopeLock {mIsLocked};
        return mData.empty();
    }

    [[nodiscard]]
    size_t ItemCount()
    {
        ScopeLock scopeLock {mIsLocked};
        return mData.size();
    }

private:
    std::atomic<bool> mIsLocked = false;
    std::queue<T> mData {};
};

}
