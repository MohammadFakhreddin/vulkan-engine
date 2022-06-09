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
    
    //-------------------------------------------------------------------------------------------------

    // Callback might be null
    TaskTracker::TaskTracker(size_t const totalTaskCount, OnFinishCallback finishCallback)
        : mFinishCallback(std::move(finishCallback))
        , mRemainingTasksCount(static_cast<int>(totalTaskCount))
    {
        MFA_ASSERT(mRemainingTasksCount > 0);
    }
    
    //-------------------------------------------------------------------------------------------------

    TaskTracker::TaskTracker(size_t const totalTaskCount)
        : mRemainingTasksCount(static_cast<int>(totalTaskCount))
    {}

    //-------------------------------------------------------------------------------------------------

    TaskTracker::~TaskTracker() = default;
    
    //-------------------------------------------------------------------------------------------------

    void TaskTracker::onComplete()
    {
        --mRemainingTasksCount;
        MFA_ASSERT(mRemainingTasksCount >= 0);
        if (mRemainingTasksCount == 0 && mFinishCallback != nullptr)
        {
            mFinishCallback();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TaskTracker::setFinishCallback(OnFinishCallback const & finishCallback)
    {
        MFA_ASSERT(finishCallback != nullptr);
        mFinishCallback = finishCallback;
    }
    ;
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
