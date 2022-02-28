#include "ThreadPool.hpp"

namespace MFA::JobSystem
{

    //-------------------------------------------------------------------------------------------------

    ThreadPool::ThreadPool()
    {
        mMainThreadId = std::this_thread::get_id();
        mNumberOfThreads = std::thread::hardware_concurrency();
        if (mNumberOfThreads < 2)
        {
            mIsAlive = false;
        }
        else
        {
            mIsAlive = true;
            for (ThreadNumber threadIndex = 0; threadIndex < mNumberOfThreads; threadIndex++)
            {
                mThreadObjects.emplace_back(std::make_unique<ThreadObject>(threadIndex, *this));
            }
        }
    }

    //-------------------------------------------------------------------------------------------------

    bool ThreadPool::IsMainThread() const
    {
        return std::this_thread::get_id() == mMainThreadId;
    }

    //-------------------------------------------------------------------------------------------------

    ThreadPool::~ThreadPool()
    {
        mIsAlive = false;
        for (auto const & thread : mThreadObjects)
        {
            thread->Notify();
        }
        for (auto const & thread : mThreadObjects)
        {
            thread->Join();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void ThreadPool::AssignTask(Task const & task) const
    {
        assert(task != nullptr);

        if (mIsAlive == true)
        {
            ThreadObject * freeThread = nullptr;
            size_t freeThreadTaskCount = 0;
            for (auto & thread : mThreadObjects)
            {
                if (freeThread == nullptr || freeThreadTaskCount > thread->InQueueTasksCount())
                {
                    freeThread = thread.get();
                    freeThreadTaskCount = thread->InQueueTasksCount();
                }
            }
            MFA_ASSERT(freeThread != nullptr);
            freeThread->Assign(task);
        }
        else
        {
            task(0, 1);
        }
    }

    //-------------------------------------------------------------------------------------------------

    void ThreadPool::AssignTaskPerThread(Task const & task) const
    {
        assert(task != nullptr);

        if (mIsAlive == true)
        {
            for (auto & thread : mThreadObjects)
            {
                thread->Assign(task);
            }
        }
        else
        {
            task(0, 1);
        }
    }

    //-------------------------------------------------------------------------------------------------

    ThreadPool::ThreadObject::ThreadObject(ThreadNumber const threadNumber, ThreadPool & parent)
        :
        mParent(parent),
        mThreadNumber(threadNumber)
    {
        mThread = std::make_unique<std::thread>([this]()-> void
        {
            mainLoop();
        });
    }

    //-------------------------------------------------------------------------------------------------

    auto ThreadPool::ThreadObject::Assign(Task const & task)->void
    {
        while (mTasks.TryToPush(task) == false);
        mCondition.notify_one();
    }

    //-------------------------------------------------------------------------------------------------

    ThreadNumber ThreadPool::GetNumberOfAvailableThreads() const
    {
        return mNumberOfThreads;
    }

    //-------------------------------------------------------------------------------------------------

    bool ThreadPool::ThreadObject::awakeCondition()
    {
        return mTasks.IsEmpty() == false || mParent.mIsAlive == false;
    }

    //-------------------------------------------------------------------------------------------------

    void ThreadPool::ThreadObject::Join() const
    {
        mThread->join();
    }

    //-------------------------------------------------------------------------------------------------

    bool ThreadPool::ThreadObject::IsFree()
    {
        auto const isFree = mTasks.IsEmpty() && mIsBusy == false;
        if (isFree == false)
        {
            mCondition.notify_one();
        }
        return isFree;
    }

    //-------------------------------------------------------------------------------------------------

    size_t ThreadPool::ThreadObject::InQueueTasksCount()
    {
        return mTasks.ItemCount();
    }

    //-------------------------------------------------------------------------------------------------

    void ThreadPool::ThreadObject::Notify()
    {
        mCondition.notify_one();
    }

    //-------------------------------------------------------------------------------------------------

    ThreadNumber ThreadPool::ThreadObject::GetThreadNumber() const
    {
        return mThreadNumber;
    }

    //-------------------------------------------------------------------------------------------------

    void ThreadPool::ThreadObject::mainLoop()
    {
        std::mutex mutex{};
        std::unique_lock<std::mutex> mLock{ mutex };
        while (mParent.mIsAlive)
        {
            mCondition.wait(mLock, [this]()->bool
                {
                    return awakeCondition();
                }
            );
            mIsBusy = true;
            while (mTasks.IsEmpty() == false && mParent.mIsAlive)
            {
                Task currentTask;
                while (mTasks.TryToPop(currentTask) == false);
                try
                {
                    if (currentTask != nullptr)
                    {
                        currentTask(mThreadNumber, mParent.mNumberOfThreads);
                    }
                }
                catch (std::exception const & exception)
                {
                    while (mParent.exceptions.TryToPush(exception.what()) == false);
                }
            }
            mIsBusy = false;
        }
    }

    //-------------------------------------------------------------------------------------------------

    bool ThreadPool::mainThreadAwakeCondition() const
    {
        bool shouldAwake = true;
        for (auto const & threadObject : mThreadObjects)
        {
            if (threadObject->IsFree() == false)
            {
                threadObject->Notify();
                shouldAwake = false;
            }
        }
        return shouldAwake;
    }

    //-------------------------------------------------------------------------------------------------

    void ThreadPool::WaitForThreadsToFinish()
    {
        assert(std::this_thread::get_id() == mMainThreadId);

        //When number of threads is 2 it means that platform only uses main thread
        while (mainThreadAwakeCondition() == false)
        {
            for (auto const & threadObject : mThreadObjects)
            {
                threadObject->Notify();
            }
            std::this_thread::yield();
        }
        
        assert(AllThreadsAreIdle() == true);

        while (!exceptions.IsEmpty())
        {
            std::string exceptionHolder;
            while (exceptions.TryToPop(exceptionHolder) == false);
            printf("%s", exceptionHolder.c_str());
        }
    }

    //-------------------------------------------------------------------------------------------------

    bool ThreadPool::AllThreadsAreIdle()
    {
        for (auto const & threadObject : mThreadObjects)
        {
            if (threadObject->IsFree() == false)
            {
                return false;
            }
        }
        return true;
    }

    //-------------------------------------------------------------------------------------------------

}
