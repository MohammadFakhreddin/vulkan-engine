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
        if (mLock.compare_exchange_strong(expectedValue, desiredValue) == false) {
            return false;
        }

        MFA_ASSERT(mLock == true);
        mData.push(newData);
        mLock = false;
        return true;
    }

    void Push(T newData)
    {
        while (TryToPush(newData) == false);
    }

    // Returns front item
    bool TryToPop(T & outData) {
        bool expectedValue = false;
        bool const desiredValue = true;
        if (mLock.compare_exchange_strong(expectedValue, desiredValue) == false) {
            return false;
        }

        MFA_ASSERT(mLock == true);
        bool success = false;
        if (mData.empty() == false) {
            outData = mData.front();
            mData.pop();
            success = true;
        }

        mLock = false;
        return success;
    }

    void Pop(T & outData)
    {
        while (TryToPop(outData) == false);
    }

    [[nodiscard]]
    bool IsEmpty() {
        SCOPE_LOCK(mLock)
        return mData.empty();
    }

    [[nodiscard]]
    size_t ItemCount()
    {
        SCOPE_LOCK(mLock)
        return mData.size();
    }

private:
    std::atomic<bool> mLock = false;
    std::queue<T> mData {};
};

}
