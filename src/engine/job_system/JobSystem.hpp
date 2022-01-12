#pragma once

#include <cstdint>
#include <functional>

namespace MFA::JobSystem
{

    using ThreadNumber = uint32_t;
    using Task = std::function<void()>;

    void Init();

    void AssignTaskToAllThreads(
        Task const & task
    );

    void AssignTaskManually(
        uint32_t threadNumber,
        Task const & task
    );

    void AutoAssignTask(Task const & task);

    [[nodiscard]]
    uint32_t GetNumberOfAvailableThreads();

    void WaitForThreadsToFinish();

    void Shutdown();

}


namespace MFA
{
    namespace JS = JobSystem;
}
