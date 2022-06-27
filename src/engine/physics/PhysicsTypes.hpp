#pragma once

#include "engine/BedrockAssert.hpp"

#include <memory>

namespace MFA::Physics
{
    template<typename T>
    class Handle {
    public:

        explicit Handle() = default;

        explicit Handle(T * ptr_) : ptr(ptr_)
        {
            MFA_ASSERT(ptr != nullptr);
        }

        ~Handle()
        {
            if (ptr != nullptr)
            {
                ptr->release();
                ptr = nullptr;
            }
        }

        [[nodiscard]]
        T * Ptr() const
        {
            return ptr;
        }

        [[nodiscard]]
        T & Ref() const
        {
            return *ptr;
        }

        [[nodiscard]]
        T & operator*() const noexcept {
            return *ptr;
        }

        [[nodiscard]]
        T * operator->() const noexcept {
            return ptr;
        }

        [[nodiscard]]
        explicit operator bool() const noexcept {
            return ptr != nullptr;
        }

    private:

        T * ptr = nullptr;

    };

    template<typename T>
    using SharedHandle = std::shared_ptr<Handle<T>>;

    template<typename T>
    SharedHandle<T> CreateHandle(T * ptr)
    {
        MFA_ASSERT(ptr != nullptr);
        return  std::make_shared<Handle<T>>(ptr);
    }

}
