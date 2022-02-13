#include "JobSystem.hpp"

#include "ThreadPool.hpp"
#include "engine/BedrockAssert.hpp"

namespace MFA::JobSystem
{

    struct State
    {
        ThreadPool threadPool {};
    };
    State * state = nullptr;

    class TaskTracker
    {
    public:

        // Callback might be null
        explicit TaskTracker(size_t const totalTaskCount, OnFinishCallback finishCallback)
            : mFinishCallback(std::move(finishCallback))
            , mRemainingTasksCount(static_cast<int>(totalTaskCount))
        {
            MFA_ASSERT(mRemainingTasksCount > 0);
        }

        ~TaskTracker() = default;

        TaskTracker(TaskTracker const &) noexcept = delete;
        TaskTracker(TaskTracker &&) noexcept = delete;
        TaskTracker & operator = (TaskTracker const &) noexcept = delete;
        TaskTracker & operator = (TaskTracker &&) noexcept = delete;

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

        OnFinishCallback mFinishCallback;
        std::atomic<int> mRemainingTasksCount;

    };

    //-------------------------------------------------------------------------------------------------

    void Init()
    {
        MFA_ASSERT(state == nullptr);
        state = new State();
    }

    //-------------------------------------------------------------------------------------------------

    void AssignTask(std::vector<Task> const & tasks, OnFinishCallback const & onTaskFinished)
    {
        MFA_ASSERT(tasks.empty() == false);

        auto const tasksCount = tasks.size();
        
        std::shared_ptr<TaskTracker> tracker = std::make_shared<TaskTracker>(tasksCount, onTaskFinished);

        for (auto & task : tasks)
        {
            state->threadPool.AssignTask([tracker, task](ThreadNumber const threadNumber, ThreadNumber const threadCount)->void{

                MFA_ASSERT(task != nullptr);
                task(threadNumber, threadCount);
                MFA_ASSERT(tracker != nullptr);
                tracker->onComplete();
            });
        }
    }

    //-------------------------------------------------------------------------------------------------

    //void AssignTaskToAllThreads(Task const & task)
    //{
        //state->threadPool.AssignTaskToAllThreads(task);
    //}

    //-------------------------------------------------------------------------------------------------

    //void AssignTaskManually(uint32_t const threadNumber, Task const & task)
    //{
        //state->threadPool.AssignTask(threadNumber, task);
    //}

    //-------------------------------------------------------------------------------------------------

    //void AutoAssignTask(Task const & task)
    //{
    //    // TODO Track thread status to assign to the thread that is under the least pressure.
    //    static uint32_t nextThreadNumber = 0;
    //    JS::AssignTaskManually(nextThreadNumber, task);
    //    ++nextThreadNumber;
    //    if (nextThreadNumber >= state->threadPool.GetNumberOfAvailableThreads())
    //    {
    //        nextThreadNumber = 0;
    //    }
    //}

    //-------------------------------------------------------------------------------------------------

    uint32_t GetNumberOfAvailableThreads()
    {
        return state->threadPool.GetNumberOfAvailableThreads();
    }

    //-------------------------------------------------------------------------------------------------

    void WaitForThreadsToFinish()
    {
        state->threadPool.WaitForThreadsToFinish();
    }

    //-------------------------------------------------------------------------------------------------

    void Shutdown()
    {
        MFA_ASSERT(state != nullptr);
        delete state;
    }

    //-------------------------------------------------------------------------------------------------

    bool IsMainThread()
    {
        return state->threadPool.IsMainThread();
    }

    //-------------------------------------------------------------------------------------------------

}
