#include "ThreadPool.hpp"

namespace MFA
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

    void ThreadPool::AssignTaskToAllThreads(Task const & task)
    {
        assert(std::this_thread::get_id() == mMainThreadId);
        for (ThreadNumber threadIndex = 0; threadIndex < mNumberOfThreads; threadIndex++)
        {
            AssignTask(threadIndex, task);
        }
    }

    //-------------------------------------------------------------------------------------------------

    void ThreadPool::AssignTask(
        ThreadNumber const threadNumber,
        Task const & task
    )
    {
        assert(std::this_thread::get_id() == mMainThreadId);
        assert(threadNumber >= 0 && threadNumber < mNumberOfThreads);
        assert(task != nullptr);

        if (mNumberOfThreads > 1)
        {
            mThreadObjects[threadNumber]->Assign(task);
        }
        else
        {
            task();
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

    ThreadPool::ThreadNumber ThreadPool::GetNumberOfAvailableThreads() const
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
        return mTasks.IsEmpty() && mIsBusy == false;
    }

    //-------------------------------------------------------------------------------------------------

    void ThreadPool::ThreadObject::Notify()
    {
        mCondition.notify_one();
    }

    //-------------------------------------------------------------------------------------------------

    void ThreadPool::ThreadObject::mainLoop()
    {
        std::mutex mMutex{};
        std::unique_lock<std::mutex> mLock{ mMutex };
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
                        currentTask();
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

    bool ThreadPool::mainThreadAwakeCondition()
    {
        for (auto & threadObject : mThreadObjects)
        {
            if (threadObject->IsFree() == false)
            {
                return false;
            }
        }
        return true;
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
            std::this_thread::sleep_for(std::chrono::nanoseconds(50));
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
