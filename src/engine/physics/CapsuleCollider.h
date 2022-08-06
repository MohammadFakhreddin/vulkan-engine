#pragma once

#include "Collider.h"

#include "Utils/MatrixUtils.h"

// A capsule-shaped primitive collider.
// Capsules are cylinders with a half - sphere at each end.

class CapsuleCollider final : public Collider
{
public:



    void SetCenter(const glm::vec3 & center)
    {
        mCenter = center;
    } 

    [[nodiscard]] glm::vec3 GetCenter() const noexcept
    {
        return mCenter;
    }

    void SetDirection(const MatrixUtils::Direction direction)
    {
        mDirection = direction;
    } 

    [[nodiscard]] MatrixUtils::Direction GetDirection() const noexcept
    {
        return mDirection;
    }

    void SetHeight(const float height)
    {
        mHeight = height;
    }

    [[nodiscard]] float GetHeight() const noexcept
    {
        return mHeight;
    }

    void SetRadius(const float radius)
    {
        mRadius = radius;
    }

    [[nodiscard]] float GetRadius() const noexcept
    {
        return mRadius;
    }

protected:

    bool init() override;

private:

    glm::vec3 mCenter {};                       // The center of the capsule, measured in the object's local space.
    MatrixUtils::Direction mDirection = MatrixUtils::Direction::yAxis;	// The value can be 0, 1 or 2 corresponding to the X, Y and Z axes, respectively.
    float mHeight = 1;                          // The height of the capsule measured in the object's local space.
    float mRadius = 0.5f;                       // The radius of the sphere, measured in the object's local space.
};
