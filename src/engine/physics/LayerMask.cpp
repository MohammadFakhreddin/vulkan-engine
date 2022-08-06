#include "LayerMask.hpp"


namespace MFA::Physics
{

    //=================================================================================================

    LayerMask::LayerMask() = default;

    //=================================================================================================

    LayerMask::LayerMask(uint32_t const value)
        : mValue(value)
    {}

    //=================================================================================================

    bool LayerMask::operator==(LayerMask const & other) const noexcept
    {
        return IsEqual(other);
    }
    
    //=================================================================================================

    void LayerMask::operator|=(LayerMask const & other) noexcept
    {
        mValue |= other.mValue;
    }

    //=================================================================================================

    uint32_t LayerMask::GetValue() const noexcept
    {
        return mValue;
    }

    //=================================================================================================

    bool LayerMask::HasCollision(const LayerMask & other) const noexcept
    {
        return (mValue & other.mValue) > 0;
    }

    //=================================================================================================

    bool LayerMask::IsEqual(const LayerMask & other) const noexcept
    {
        return mValue == other.mValue;
    }

    //=================================================================================================

}
