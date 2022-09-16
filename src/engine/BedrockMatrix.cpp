#include "BedrockMatrix.hpp"

#include "BedrockAssert.hpp"
#include "BedrockCommon.hpp"

namespace MFA::Matrix
{


    //-------------------------------------------------------------------------------------------------

    glm::quat ToQuat(const float xDeg, const float yDeg, const float zDeg)
    {
        return glm::angleAxis(glm::radians(xDeg), glm::vec3(1.0f, 0.0f, 0.0f))
            * glm::angleAxis(glm::radians(yDeg), glm::vec3(0.0f, 1.0f, 0.0f))
            * glm::angleAxis(glm::radians(zDeg), glm::vec3(0.0f, 0.0f, 1.0f));
    }

    //-------------------------------------------------------------------------------------------------

    glm::quat ToQuat(glm::vec3 const & eulerAngles)
    {
        return ToQuat(eulerAngles.x, eulerAngles.y, eulerAngles.z);
    }

    //-------------------------------------------------------------------------------------------------

    // Returns degree
    glm::vec3 ToEulerAngles(glm::quat const & quaternion)
    {
        auto angles = glm::degrees(glm::eulerAngles(quaternion));
        if (std::fabs(angles.z) >= 90)
        {
            angles.x += 180.f;
            angles.y = 180.f - angles.y;
            angles.z += 180.f;
        }
        return angles;
    }

    //-------------------------------------------------------------------------------------------------

    #define ROTATE_EULER_ANGLE(transform, angles)                                                  \
    transform = glm::rotate(transform, glm::radians(angles[0]), glm::vec3(1.0f, 0.0f, 0.0f));      \
    transform = glm::rotate(transform, glm::radians(angles[1]), glm::vec3(0.0f, 1.0f, 0.0f));      \
    transform = glm::rotate(transform, glm::radians(angles[2]), glm::vec3(0.0f, 0.0f, 1.0f));      \

    //-------------------------------------------------------------------------------------------------

    void RotateWithEulerAngle(glm::mat4 & inOutTransform, float eulerAngles[3])
    {
        ROTATE_EULER_ANGLE(inOutTransform, eulerAngles);
    }

    //-------------------------------------------------------------------------------------------------

    void RotateWithEulerAngle(glm::mat4 & inOutTransform, glm::vec3 eulerAngles)
    {
        ROTATE_EULER_ANGLE(inOutTransform, eulerAngles);
    }

    //-------------------------------------------------------------------------------------------------

    #define ROTATE_RADIANS(transform, radians)                                        \
    transform = glm::rotate(transform, radians[0], glm::vec3(1.0f, 0.0f, 0.0f));      \
    transform = glm::rotate(transform, radians[1], glm::vec3(0.0f, 1.0f, 0.0f));      \
    transform = glm::rotate(transform, radians[2], glm::vec3(0.0f, 0.0f, 1.0f));      \

    //-------------------------------------------------------------------------------------------------

    void RotateWithRadians(glm::mat4 & inOutTransform, float radians[3])
    {
        ROTATE_RADIANS(inOutTransform, radians);
    }

    //-------------------------------------------------------------------------------------------------

    void RotateWithRadians(glm::mat4 & inOutTransform, glm::vec3 radians)
    {
        ROTATE_RADIANS(inOutTransform, radians);
    }

    //-------------------------------------------------------------------------------------------------

    void Scale(glm::mat4 & transform, float scale[3])
    {
        transform = glm::scale(
            transform,
            glm::vec3(
                scale[0],
                scale[1],
                scale[2]
            )
        );
    }

    //-------------------------------------------------------------------------------------------------

    void Scale(glm::mat4 & transform, glm::vec3 const & scale)
    {
        transform = glm::scale(
            transform,
            scale
        );
    }

    //-------------------------------------------------------------------------------------------------

    void Translate(glm::mat4 & transform, float distance[3])
    {
        transform = glm::translate(transform, glm::vec3(distance[0], distance[1], distance[2]));
    }

    //-------------------------------------------------------------------------------------------------

    void Translate(glm::mat4 & transform, glm::vec3 const & distance)
    {
        transform = glm::translate(transform, distance);
    }

    //-------------------------------------------------------------------------------------------------

    void PreparePerspectiveProjectionMatrix(
        float outMatrix[16],
        float const aspectRatio,
        float const fieldOfView,
        float const nearPlane,
        float const farPlane
    )
    {
        float const invTan = 1.0f / tan(Math::Deg2Rad(0.5f * fieldOfView));
        float const invDepthDiff = 1.0f / (nearPlane - farPlane);

        outMatrix[0] = invTan;
        outMatrix[1 * 4 + 1] = invTan * aspectRatio;
        outMatrix[2 * 4 + 2] = 1.0f * farPlane * invDepthDiff;
        outMatrix[3 * 4 + 2] = 1.0f * nearPlane * farPlane * invDepthDiff;
        outMatrix[2 * 4 + 3] = -1.0f;
    }

    //-------------------------------------------------------------------------------------------------

    void PreparePerspectiveProjectionMatrix(
        glm::mat4 & outMatrix,
        float const aspectRatio,
        float const fieldOfView,
        float const nearPlane,
        float const farPlane
    )
    {
        float const invTan = 1.0f / tan(Math::Deg2Rad(0.5f * fieldOfView));
        float const invDepthDiff = 1.0f / (nearPlane - farPlane);

        outMatrix[0][0] = invTan;
        MFA_ASSERT(outMatrix[0][1] == 0.0f);
        MFA_ASSERT(outMatrix[0][2] == 0.0f);
        MFA_ASSERT(outMatrix[0][3] == 0.0f);

        MFA_ASSERT(outMatrix[1][0] == 0.0f);
        outMatrix[1][1] = invTan * aspectRatio;
        MFA_ASSERT(outMatrix[1][2] == 0.0f);
        MFA_ASSERT(outMatrix[1][3] == 0.0f);

        MFA_ASSERT(outMatrix[2][0] == 0.0f);
        MFA_ASSERT(outMatrix[2][1] == 0.0f);
        outMatrix[2][2] = 1.0f * farPlane * invDepthDiff;
        outMatrix[2][3] = -1.0f;

        MFA_ASSERT(outMatrix[3][0] == 0.0f);
        MFA_ASSERT(outMatrix[3][1] == 0.0f);
        outMatrix[3][2] = 1.0f * nearPlane * farPlane * invDepthDiff;
        MFA_ASSERT(outMatrix[3][3] == 0.0f);

    }

    //-------------------------------------------------------------------------------------------------

    void PrepareOrthographicProjectionMatrix(
        glm::mat4 & outMatrix,
        float const leftPlane,
        float const rightPlane,
        float const bottomPlane,
        float const topPlane,
        float const nearPlane,
        float const farPlane
    )
    {
        outMatrix[0][0] = 2.0f / (rightPlane - leftPlane);
        MFA_ASSERT(outMatrix[0][1] == 0.0f);
        MFA_ASSERT(outMatrix[0][2] == 0.0f);
        MFA_ASSERT(outMatrix[0][3] == 0.0f);

        MFA_ASSERT(outMatrix[1][0] == 0.0f);
        outMatrix[1][1] = 2.0f / (bottomPlane - topPlane);
        MFA_ASSERT(outMatrix[1][2] == 0.0f);
        MFA_ASSERT(outMatrix[1][3] == 0.0f);

        MFA_ASSERT(outMatrix[2][0] == 0.0f);
        MFA_ASSERT(outMatrix[2][1] == 0.0f);
        outMatrix[2][2] = 1.0f / (nearPlane - farPlane);
        MFA_ASSERT(outMatrix[2][3] == 0.0f);

        outMatrix[3][0] = -(rightPlane + leftPlane) / (rightPlane - leftPlane);
        outMatrix[3][1] = -(bottomPlane + topPlane) / (bottomPlane - topPlane);
        outMatrix[3][2] = nearPlane / (nearPlane - farPlane);
        outMatrix[3][3] = 1.0f;
    }

    //-------------------------------------------------------------------------------------------------
    // Based on https://gamedev.stackexchange.com/questions/118960/convert-a-direction-vector-normalized-to-rotation
    glm::quat ToRotation(glm::vec3 direction)
    {
        direction = glm::normalize(direction);
        auto const angleH = std::atan2(direction.x, direction.z);
        auto const quatXZ = glm::angleAxis(angleH, Math::UpVec3);
        auto const angleV = -std::asin(direction.y);
        auto const quatY = glm::angleAxis(angleV, Math::RightVec3);
        return quatXZ * quatY;
    }

    //-------------------------------------------------------------------------------------------------

    float Angle(glm::quat const& a, glm::quat const& b)
    {
        auto const dot = glm::dot(a, b);
        if (IsEqualUsingDot(dot))
        {
            return 0.0f;
        }
        return std::acos(dot) * 2.0f;
    }

    //-------------------------------------------------------------------------------------------------

    bool IsEqualUsingDot(float const dot)
    {
        // Returns false in the presence of NaN values.
        return dot >= 1.0f - Math::Epsilon<float>();
    }

    //-------------------------------------------------------------------------------------------------

    bool IsEqual(glm::quat const& a, glm::quat const& b)
    {
        auto const dot = glm::dot(a, b);
        return IsEqualUsingDot(dot);
    }

    //-------------------------------------------------------------------------------------------------

    glm::quat RotateTowards(glm::quat const& from, glm::quat const& to, float const maxDegreeDelta)
    {
        MFA_ASSERT(maxDegreeDelta > 0.0f);

        auto cosTheta = glm::dot(from, to);
        if (IsEqualUsingDot(cosTheta))
        {
            return to;
        }

        glm::quat to2 = to;
        
        // If cosTheta < 0, the interpolation will take the long way around the sphere.
        // To fix this, one quat must be negated.
        if (cosTheta < 0.0f)
        {
            to2 = -to;
            cosTheta = -cosTheta;
        }

        float const halfTheta = Math::ACosSafe(cosTheta);
        auto const theta = halfTheta * 2.0f;

        auto const fraction = std::clamp(maxDegreeDelta / theta, 0.0f, 1.0f);

        // Perform a linear interpolation when cosTheta is close to 1 to avoid side effect of sin(angle) becoming a zero denominator
        if (cosTheta > 1.0f - Math::Epsilon<float>())
        {
            // Linear interpolation
            return glm::quat(
                glm::mix(from.w, to2.w, fraction),
                glm::mix(from.x, to2.x, fraction),
                glm::mix(from.y, to2.y, fraction),
                glm::mix(from.z, to2.z, fraction)
            );
        }
        else
        {
            // Essential Mathematics, page 467
            return (sinf((1.0f - fraction) * halfTheta) * from + sinf(fraction * halfTheta) * to2) / sinf(halfTheta);
        }
    }
    
    //-------------------------------------------------------------------------------------------------

}
