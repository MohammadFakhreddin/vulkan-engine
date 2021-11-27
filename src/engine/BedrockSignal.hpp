#pragma once

#include "engine/BedrockAssert.hpp"
#include "engine/job_system/JobSystem.hpp"

#include <functional>
#include <vector>

namespace MFA {

template<typename ... ArgsT>
class Signal
{
public:

    using ListenerId = int;
    using Listener = std::function<void(ArgsT ... args)>;

    struct Slot
    {
        ListenerId id;
        Listener listener;
    };

    template <typename Instance>
    ListenerId Register(Instance * obj, void (Instance::*memFunc)(ArgsT...))
    {
        // Member function wrapper.
        auto wrapperLambda = [obj, memFunc](ArgsT... args)
        {
            (obj->*memFunc)(std::forward<ArgsT>(args)...);
        };

        return Register(std::move(wrapperLambda));
    }

    ListenerId Register(const Listener & listener)
    {
        MFA_ASSERT(listener != nullptr);
        mSlots.emplace_back(Slot {mNextId, listener});
        ++mNextId;
        return mSlots.back().id;
    }

    bool UnRegister(ListenerId listenerId)
    {
        for (int i = static_cast<int>(mSlots.size() - 1); i >= 0; --i)
        {
            if (mSlots[i].id == listenerId)
            {
                mSlots[i] = mSlots.back();
                mSlots.pop_back();
                return true;
            }
        }
        return false;
    }

    void Emit(ArgsT ... args) const
    {
        for (int i = static_cast<int>(mSlots.size()) - 1; i >= 0; --i)
        {
            const auto & slot = mSlots[i];
            slot.listener(std::forward<ArgsT>(args)...);
        }
    }

    //template<typename ... PassedArgs>
    //void EmitMultiThread(PassedArgs && ... args) const
    //{
    //    uint32_t nextThreadNumber = 0;
    //    auto const availableThreadCount = JS::GetNumberOfAvailableThreads();
    //
    //    for (int i = static_cast<int>(mSlots.size()) - 1; i >= 0; --i)
    //    {
    //        const auto & slot = mSlots[i];
    //        JS::AssignTask(nextThreadNumber, [&slot, args...]()->void{
    //            slot.listener(std::forward<ArgsT>(args)...);
    //        });
    //        ++nextThreadNumber;
    //        if (nextThreadNumber >= availableThreadCount)
    //        {
    //            nextThreadNumber = 0;
    //        }
    //    }

    //    JS::WaitForThreadsToFinish();

    //}


    [[nodiscard]]
    bool IsEmpty() const
    {
        return mSlots.empty();
    }

private:

    std::vector<Slot> mSlots {};

    ListenerId mNextId = 0;

};

}
