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
            MFA_ASSERT(task != nullptr);
            state->threadPool.AssignTask([tracker, task](ThreadNumber const threadNumber, ThreadNumber const threadCount)->void{
                MFA_ASSERT(task != nullptr);
                task(threadNumber, threadCount);
                MFA_ASSERT(tracker != nullptr);
                tracker->onComplete();
            });
        }
    }
    
    //-------------------------------------------------------------------------------------------------

    void AssignTask(Task const & task, OnFinishCallback const & onTaskFinished)
    {
        MFA_ASSERT(task != nullptr);

        AssignTask(std::vector<Task>{task}, onTaskFinished);
    }

    //-------------------------------------------------------------------------------------------------

    void AssignTaskPerThread(Task const & task)
    {
        MFA_ASSERT(task != nullptr);
        state->threadPool.AssignTaskPerThread(task);
    }

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
        MFA_ASSERT(state != nullptr);
        return state->threadPool.IsMainThread();
    }

    //-------------------------------------------------------------------------------------------------

}
