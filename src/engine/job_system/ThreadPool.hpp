#pragma once

#include "ThreadSafeQueue.hpp"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>

namespace MFA {

class ThreadPool
{
    using ThreadNumber = uint32_t;
    using Task = std::function<void()>;
public:

    explicit ThreadPool();
    // We can have a threadPool with custom number of threads

    ~ThreadPool();

    ThreadPool (ThreadPool const &) noexcept = delete;
    ThreadPool (ThreadPool &&) noexcept = delete;
    ThreadPool & operator = (ThreadPool const &) noexcept = delete;
    ThreadPool & operator = (ThreadPool &&) noexcept = delete;

    bool IsMainThread() const;

    void AssignTaskToAllThreads(
        Task const & task
    );

    void AssignTask(
        uint32_t threadNumber,
        Task const & task
    );

    [[nodiscard]]
    uint32_t GetNumberOfAvailableThreads() const;

    void WaitForThreadsToFinish();

    class ThreadObject{

    public:

        explicit ThreadObject(ThreadNumber threadNumber, ThreadPool & parent);

        ~ThreadObject() = default;

        ThreadObject (ThreadObject const &) noexcept = delete;
        ThreadObject (ThreadObject &&) noexcept = delete;
        ThreadObject & operator = (ThreadObject const &) noexcept = delete;
        ThreadObject & operator = (ThreadObject &&) noexcept = delete;

        void Assign(Task const & task);

        void Join() const;

        [[nodiscard]]
        bool IsFree();

        void Notify();

        [[nodiscard]]
        ThreadNumber GetThreadNumber() const
        {
            return mThreadNumber;
        }

    private:

        void mainLoop();
        
        bool awakeCondition();

        ThreadPool & mParent;

        ThreadNumber mThreadNumber;

        ThreadSafeQueue<Task> mTasks;

        std::condition_variable mCondition;

        std::unique_ptr<std::thread> mThread;

        bool mIsBusy = false;

    };

    bool AllThreadsAreIdle();

private:
  
    bool mainThreadAwakeCondition();

    std::vector<std::unique_ptr<ThreadObject>> mThreadObjects;

    bool mIsAlive = true;

    uint32_t mNumberOfThreads = 0;
    
    ThreadSafeQueue<std::string> exceptions {};

    std::thread::id mMainThreadId {};

};

}
