#pragma once

#include <atomic>

namespace MFA {
    class ScopeLock
    {
    public:
        explicit ScopeLock(std::atomic<bool> & lock);
        ~ScopeLock();

        ScopeLock(ScopeLock const &) noexcept = delete;
        ScopeLock(ScopeLock &&) noexcept = delete;
        ScopeLock & operator = (ScopeLock const &) noexcept = delete;
        ScopeLock & operator = (ScopeLock &&) noexcept = delete;

    private:
        std::atomic<bool> & mLock;
    };
}
