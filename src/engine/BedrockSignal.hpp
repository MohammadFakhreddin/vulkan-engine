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
            SCOPE_LOCK(mLock)

            MFA_ASSERT(listener != nullptr);

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
                SCOPE_LOCK(mLock)

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
            std::vector<Listener> listeners {};
            {
                SCOPE_LOCK(mLock)
                for (int i = static_cast<int>(mSlots.size()) - 1; i >= 0; --i)
                {
                    const auto & slot = mSlots[i];
                    MFA_ASSERT(mSlots[i].listener != nullptr);
                    listeners.emplace_back(mSlots[i].listener);
                }
            }

            for (auto & listener : listeners)
            {
                listener(std::forward<ArgsT>(args)...);
            }
        }

        void EmitMultiThread(ArgsT ... args)
        {
            std::vector<Listener> listeners {};
            {
                SCOPE_LOCK(mLock)
                for (int i = static_cast<int>(mSlots.size()) - 1; i >= 0; --i)
                {
                    const auto & slot = mSlots[i];
                    MFA_ASSERT(mSlots[i].listener != nullptr);
                    listeners.emplace_back(mSlots[i].listener);
                }
            }

            if (listeners.empty() == false)
            {
                JS::AssignTaskPerThread(
                    [... args = std::forward<ArgsT>(args), listeners, this]
                    (uint32_t const threadNumber, uint32_t const threadCount)->void
                    {
                        for (uint32_t i = threadNumber; i < static_cast<uint32_t>(listeners.size()); i += threadCount)
                        {
                            MFA_ASSERT(listeners[i] != nullptr);
                            listeners[i](args...);
                        }
                    }
                );

                // TODO: Create a config for it
                if (JS::IsMainThread())
                {
                    JS::WaitForThreadsToFinish();
                }
            }
        }

        [[nodiscard]]
        bool IsEmpty()
        {
            SCOPE_LOCK(mLock)
            return mSlots.empty();
        }

    private:

        std::vector<Slot> mSlots{};

        SignalId mNextId = 0;

        std::atomic<bool> mLock = false;

    };

}
