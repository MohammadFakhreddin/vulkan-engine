#pragma once

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockSignalTypes.hpp"
#include "engine/job_system/ScopeLock.hpp"
#include "engine/job_system/JobSystem.hpp"

#include <functional>
#include <vector>

// TODO Make this file thread safe by having a queue of tasks
namespace MFA
{

    template<typename ... ArgsT>
    class Signal
    {
    public:

        using Listener = std::function<void(ArgsT ... args)>;

        struct Slot
        {
            SignalId id;
            Listener listener;
        };

        template <typename Instance>
        SignalId Register(Instance * obj, void (Instance:: * memFunc)(ArgsT...))
        {
            // Member function wrapper.
            auto wrapperLambda = [obj, memFunc](ArgsT... args)
            {
                (obj->*memFunc)(std::forward<ArgsT>(args)...);
            };

            return Register(std::move(wrapperLambda));
        }

        SignalId Register(const Listener & listener)
        {
            MFA_ASSERT(listener != nullptr);
            ScopeLock scopeLock {mLock};
            mSlots.emplace_back(Slot{ mNextId, listener });
            ++mNextId;
            MFA_ASSERT(mNextId != InvalidSignalId);
            auto const id = mSlots.back().id;
            return id;
        }

        bool UnRegister(SignalId listenerId)
        {
            if (listenerId != InvalidSignalId)
            {
                ScopeLock scopeLock {mLock};

                for (int i = static_cast<int>(mSlots.size() - 1); i >= 0; --i)
                {
                    if (mSlots[i].id == listenerId)
                    {
                        mSlots[i] = mSlots.back();
                        mSlots.pop_back();
                        return true;
                    }
                }
            }
            return false;
        }

        void Emit(ArgsT ... args)
        {
            ScopeLock scopeLock {mLock};
            for (int i = static_cast<int>(mSlots.size()) - 1; i >= 0; --i)
            {
                const auto & slot = mSlots[i];
                slot.listener(std::forward<ArgsT>(args)...);
            }
        }

        void EmitMultiThread(ArgsT ... args)
        {
            MFA_ASSERT(JS::IsMainThread()); // We should only call this function on mainThread
            ScopeLock scopeLock {mLock};

            std::vector<JS::Task> tasks (mSlots.size());
            for (int i = 0; i < static_cast<int>(mSlots.size()); ++i)
            {
                Slot slot = mSlots[i];
                JS::Task task = [slot, args](JS::ThreadNumber threadNumber, JS::ThreadNumber threadCount)->void{
                    slot.listener(std::forward<ArgsT>(args)...);
                };
                tasks.emplace_back(task);
            }
            JobSystem::AssignTask(tasks);

            JS::WaitForThreadsToFinish();

        }


        [[nodiscard]]
        bool IsEmpty() const
        {
            return mSlots.empty();
        }

    private:

        std::vector<Slot> mSlots{};

        SignalId mNextId = 0;

        std::atomic<bool> mLock = false;

    };

}
