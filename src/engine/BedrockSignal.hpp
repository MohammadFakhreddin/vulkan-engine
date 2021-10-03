#pragma once

#include "engine/BedrockAssert.hpp"

#include <functional>

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

    template<typename ... PassedArgs>
    void Emit(PassedArgs && ... args) const
    {
        for (int i = static_cast<int>(mSlots.size()) - 1; i >= 0; --i)
        {
            const auto & slot = mSlots[i];
            slot.listener(std::forward<ArgsT>(args)...);
        }
    }

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
