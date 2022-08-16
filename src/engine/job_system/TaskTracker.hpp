#pragma once

#include "engine/BedrockAssert.hpp"

#include <functional>
#include <atomic>
#include <memory>

namespace MFA::JobSystem {
    // Do not reference to taskTracker on inside
    template<typename T>
    class TaskTracker1 final
    {
    public:

        using OnComplete = std::function<void(T const * userData)>;

        // Callback might be null
        explicit TaskTracker1(
            std::shared_ptr<T> userData,
            int const totalTaskCount,
            OnComplete finishCallback
        )
            : userData(userData)
            , mFinishCallback(std::move(finishCallback))
            , mRemainingTasksCount(totalTaskCount)
        {}

        ~TaskTracker1() = default;

        TaskTracker1(TaskTracker1 const &) noexcept = delete;
        TaskTracker1(TaskTracker1 &&) noexcept = delete;
        TaskTracker1 & operator = (TaskTracker1 const &) noexcept = delete;
        TaskTracker1 & operator = (TaskTracker1 &&) noexcept = delete;

        void onComplete()
        {
            --mRemainingTasksCount;
            MFA_ASSERT(mRemainingTasksCount >= 0);
            if (mRemainingTasksCount <= 0 && mFinishCallback != nullptr)
            {
                mFinishCallback(userData.get());
            }
        }

        std::shared_ptr<T> userData;
        
    private:

        OnComplete mFinishCallback;
        std::atomic<int> mRemainingTasksCount;

    };

    class TaskTracker2 final
    {
    public:

        using OnComplete = std::function<void()>;

        // Callback might be null
        explicit TaskTracker2(
            int const totalTaskCount,
            OnComplete finishCallback
        )
            : mFinishCallback(std::move(finishCallback))
            , mRemainingTasksCount(totalTaskCount)
        {
            MFA_ASSERT(totalTaskCount > 0);
        }

        ~TaskTracker2() = default;

        TaskTracker2(TaskTracker2 const &) noexcept = delete;
        TaskTracker2(TaskTracker2 &&) noexcept = delete;
        TaskTracker2 & operator = (TaskTracker2 const &) noexcept = delete;
        TaskTracker2 & operator = (TaskTracker2 &&) noexcept = delete;

        void onComplete()
        {
            --mRemainingTasksCount;
            MFA_ASSERT(mRemainingTasksCount >= 0);
            if (mRemainingTasksCount == 0 && mFinishCallback != nullptr)
            {
                mFinishCallback();
            }
        }
        
    private:

        OnComplete mFinishCallback;
        std::atomic<int> mRemainingTasksCount;

    };
}

namespace MFA
{
    namespace JS = JobSystem;
}
