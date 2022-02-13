#pragma once

#include "JobSystemTypes.hpp"

namespace MFA::JobSystem
{

    void Init();

    // TODO All jobs should return a ticket
    //void AssignTaskToAllThreads(Task const & task);

    /*void AssignTaskManually(
        uint32_t threadNumber,
        Task const & task
    );*/

    //void AutoAssignTask(Task const & task);

    void AssignTask(std::vector<Task> const & tasks, OnFinishCallback const & onTaskFinished = nullptr);

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
