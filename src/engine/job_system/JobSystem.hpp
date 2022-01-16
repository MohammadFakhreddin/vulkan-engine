#pragma once

#include <cstdint>
#include <functional>

namespace MFA::JobSystem
{
    // TODO Try to use std::async for multi-threading
    using ThreadNumber = uint32_t;
    using Task = std::function<void()>;

    void Init();

    void AssignTaskToAllThreads(Task const & task);

    void AssignTaskManually(
        uint32_t threadNumber,
        Task const & task
    );

    void AutoAssignTask(Task const & task);

    // Runs on main thread
    void RunOnPostRender(Task const & task);

    [[nodiscard]]
    uint32_t GetNumberOfAvailableThreads();

    void WaitForThreadsToFinish();

    void Shutdown();

    void OnPostRender();

}


namespace MFA
{
    namespace JS = JobSystem;
}
