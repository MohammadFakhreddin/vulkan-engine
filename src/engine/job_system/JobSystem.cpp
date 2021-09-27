#include "JobSystem.hpp"

#include "ThreadPool.hpp"
#include "engine/BedrockAssert.hpp"

namespace MFA::JobSystem {

// TODO We need token functionality

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

void AssignTask(uint32_t const threadNumber, Task const & task)
{
    state->threadPool.AssignTask(threadNumber, task);
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
