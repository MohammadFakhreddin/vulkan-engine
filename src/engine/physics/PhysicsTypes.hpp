#pragma once

#include "engine/BedrockAssert.hpp"

#include "engine/BedrockMemory.hpp"

#include <glm/vec3.hpp>
#include <foundation/PxVec3.h>

#include <memory>
#include <vector>

namespace physx
{
    class PxTriangleMesh;
}

namespace MFA::Physics
{

    struct InitParams
    {
        physx::PxVec3 gravity{};
    };

    template<typename T>
    class Handle {
    public:

        explicit Handle() = default;

        explicit Handle(T * ptr_) : ptr(ptr_)
        {
            MFA_ASSERT(ptr != nullptr);
        }

        Handle (Handle const &) noexcept = delete;             
        Handle (Handle &&) noexcept = delete; 
        Handle & operator = (Handle const &) noexcept = delete;
        Handle & operator = (Handle && rhs) noexcept = delete;

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

    struct TriangleMeshDesc
    {
        std::shared_ptr<SmartBlob> pointsBuffer;
        uint32_t pointsCount;
        uint32_t pointsStride;

        std::shared_ptr<SmartBlob> triangleBuffer;  // 3 Indices together creates a triangle stride
        uint32_t trianglesCount;
        uint32_t trianglesStride;
    };

    struct TriangleMeshGroup
    {
        std::vector<SharedHandle<physx::PxTriangleMesh>> triangleMeshes {};
    };
    
}
