#pragma once

#include "ThreadSafeQueue.hpp"
#include "JobSystemTypes.hpp"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>

namespace MFA::JobSystem
{

    class ThreadPool
    {
    public:

        explicit ThreadPool();
        // We can have a threadPool with custom number of threads

        ~ThreadPool();

        ThreadPool(ThreadPool const &) noexcept = delete;
        ThreadPool(ThreadPool &&) noexcept = delete;
        ThreadPool & operator = (ThreadPool const &) noexcept = delete;
        ThreadPool & operator = (ThreadPool &&) noexcept = delete;

        bool IsMainThread() const;

        void AssignTask(Task const & task) const;

        void AssignTaskPerThread(Task const & task) const;

        [[nodiscard]]
        ThreadNumber GetNumberOfAvailableThreads() const;

        void WaitForThreadsToFinish();

        class ThreadObject
        {
        public:

            explicit ThreadObject(ThreadNumber threadNumber, ThreadPool & parent);

            ~ThreadObject() = default;

            ThreadObject(ThreadObject const &) noexcept = delete;
            ThreadObject(ThreadObject &&) noexcept = delete;
            ThreadObject & operator = (ThreadObject const &) noexcept = delete;
            ThreadObject & operator = (ThreadObject &&) noexcept = delete;

            void Assign(Task const & task);

            void Join() const;

            [[nodiscard]]
            bool IsFree();

            [[nodiscard]]
            size_t InQueueTasksCount();

            void Notify();

            [[nodiscard]]
            ThreadNumber GetThreadNumber() const;

            bool awakeCondition();

        private:

            void mainLoop();
            
            ThreadPool & mParent;

            ThreadNumber mThreadNumber;

            ThreadSafeQueue<Task> mTasks;

            std::condition_variable mCondition;

            std::unique_ptr<std::thread> mThread;

            std::atomic<bool> mIsBusy = false;

        };

        bool AllThreadsAreIdle();

    private:

        bool mainThreadAwakeCondition() const;

        std::vector<std::unique_ptr<ThreadObject>> mThreadObjects;

        bool mIsAlive = true;

        ThreadNumber mNumberOfThreads = 0;

        ThreadSafeQueue<std::string> exceptions{};

        std::thread::id mMainThreadId{};

    };

}
