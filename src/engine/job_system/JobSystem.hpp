#pragma once

#include "JobSystemTypes.hpp"

namespace MFA::JobSystem
{

    class TaskTracker
    {
    public:

        // Callback might be null
        explicit TaskTracker(size_t totalTaskCount, OnFinishCallback finishCallback);

        explicit TaskTracker(size_t totalTaskCount);

        ~TaskTracker();

        TaskTracker(TaskTracker const &) noexcept = delete;
        TaskTracker(TaskTracker &&) noexcept = delete;
        TaskTracker & operator = (TaskTracker const &) noexcept = delete;
        TaskTracker & operator = (TaskTracker &&) noexcept = delete;

        void onComplete();

        void setFinishCallback(OnFinishCallback const & finishCallback);

    private:

        OnFinishCallback mFinishCallback;
        std::atomic<int> mRemainingTasksCount;

    };

    void Init();

    void AssignTask(std::vector<Task> const & tasks, OnFinishCallback const & onTaskFinished = nullptr);

    void AssignTask(Task const & task, OnFinishCallback const & onTaskFinished = nullptr);

    void AssignTaskPerThread(Task const & task);
    
    [[nodiscard]]
    uint32_t GetNumberOfAvailableThreads();

    void WaitForThreadsToFinish();

    void Shutdown();

    bool IsMainThread();
    
}


namespace MFA
{
    namespace JS = JobSystem;
}
