#pragma once

#include <cstdint>

namespace MFA::Physics
{

    class LayerMask
    {
    public:

        explicit LayerMask();

        explicit LayerMask(uint32_t value);

        [[nodiscard]]
        bool operator == (LayerMask const & other) const noexcept;

        void operator |= (LayerMask const & other) noexcept;

        [[nodiscard]]
        uint32_t GetValue() const noexcept;

        [[nodiscard]]
        bool HasCollision(const LayerMask & other) const noexcept;

        [[nodiscard]]
        bool IsEqual(const LayerMask & other) const noexcept;

    private:

        uint32_t mValue{};

    };

};
