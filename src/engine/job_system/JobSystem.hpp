#pragma once

#include "JobSystemTypes.hpp"

namespace MFA::JobSystem
{

    void Init();

    void AssignTask(std::vector<Task> const & tasks, OnFinishCallback const & onTaskFinished = nullptr);

    void AssignTask(Task const & task, OnFinishCallback const & onTaskFinished = nullptr);

    void AssignTaskPerThread(Task const & task, OnFinishCallback const & onTaskFinished = nullptr);
    
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
