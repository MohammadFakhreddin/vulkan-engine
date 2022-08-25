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

}
