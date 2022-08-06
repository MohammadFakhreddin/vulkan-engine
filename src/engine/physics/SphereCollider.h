#pragma once

#include "Collider.h"

// A sphere-shaped primitive collider.

class SphereCollider final : public Collider
{
public:

    void SetRadius(const float radius)
    {
        mRadius = radius;
    }

    [[nodiscard]]
    float GetRadius() const noexcept
    {
        return mRadius;
    }

protected:

    bool init() override;

private:

    float mRadius;			// The radius of the sphere measured in the object's local space.
};

