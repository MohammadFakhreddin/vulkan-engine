#include "JobSystem.hpp"

#include "ThreadPool.hpp"
#include "engine/BedrockAssert.hpp"

namespace MFA::JobSystem
{

    // TODO We need token functionality or we can use std::async

    struct State
    {
        ThreadPool threadPool {};
    };
    State * state = nullptr;

    //-------------------------------------------------------------------------------------------------

    void Init()
    {
        MFA_ASSERT(state == nullptr);
        state = new State();
    }

    //-------------------------------------------------------------------------------------------------

    void AssignTaskToAllThreads(Task const & task)
    {
        state->threadPool.AssignTaskToAllThreads(task);
    }

    //-------------------------------------------------------------------------------------------------

    void AssignTaskManually(uint32_t const threadNumber, Task const & task)
    {
        state->threadPool.AssignTask(threadNumber, task);
    }

    //-------------------------------------------------------------------------------------------------

    void AutoAssignTask(Task const & task)
    {
        // TODO Track thread status to assign to the thread that is under the least pressure.
        static uint32_t nextThreadNumber = 0;
        JS::AssignTaskManually(nextThreadNumber, task);
        ++nextThreadNumber;
        if (nextThreadNumber >= state->threadPool.GetNumberOfAvailableThreads())
        {
            nextThreadNumber = 0;
        }
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

}
