#include "BedrockMatrix.hpp"

#include "BedrockAssert.hpp"
#include "BedrockMath.hpp"

namespace MFA::Matrix
{

    //-------------------------------------------------------------------------------------------------

    glm::mat4 CopyCellsToMat4(float const * cells) {
        MFA_ASSERT(cells != nullptr);
        glm::mat4 result {};
        CopyCellsToGlm(cells, result);
        return result;
    }

    //-------------------------------------------------------------------------------------------------

    glm::vec3 CopyCellsToVec3(float const * cells) {
        MFA_ASSERT(cells != nullptr);
        glm::vec3 result;
        CopyCellsToGlm(cells, result);
        return result;
    }

    //-------------------------------------------------------------------------------------------------

    glm::quat CopyCellsToQuat(float const * cells) {
        MFA_ASSERT(cells != nullptr);
        glm::quat result;
        CopyCellsToGlm(cells, result);
        return result;
    }

    //-------------------------------------------------------------------------------------------------

    glm::vec4 CopyCellsToVec4(float const * cells) {
        MFA_ASSERT(cells != nullptr);
        glm::vec4 result;
        CopyCellsToGlm(cells, result);
        return result;
    }

    //-------------------------------------------------------------------------------------------------

    void CopyCellsToGlm(float const * cells, glm::mat4 & outMatrix)
    {
        MFA_ASSERT(cells != nullptr);
        ::memcpy(&outMatrix, cells, sizeof(float) * 16);
    }

    //-------------------------------------------------------------------------------------------------

    void CopyCellsToGlm(float const * cells, glm::vec3 & outVec3)
    {
        MFA_ASSERT(cells != nullptr);
        ::memcpy(&outVec3, cells, sizeof(float) * 3);
    }

    //-------------------------------------------------------------------------------------------------

    void CopyCellsToGlm(float const * cells, glm::quat & outQuat)
    {
        MFA_ASSERT(cells != nullptr);
        ::memcpy(&outQuat, cells, sizeof(float) * 4);
    }

    //-------------------------------------------------------------------------------------------------

    void CopyCellsToGlm(float const * cells, glm::vec4 & outVec4)
    {
        MFA_ASSERT(cells != nullptr);
        ::memcpy(&outVec4, cells, sizeof(float) * 4);
    }

    //-------------------------------------------------------------------------------------------------

    void CopyGlmToCells(glm::mat4 const & matrix, float * cells)
    {
        MFA_ASSERT(cells != nullptr);
        ::memcpy(cells, &matrix, sizeof(float) * 16);
    }

    //-------------------------------------------------------------------------------------------------

    void CopyGlmToCells(glm::vec2 const & vector, float * cells)
    {
        MFA_ASSERT(cells != nullptr);
        ::memcpy(cells, &vector, 2 * sizeof(float));
    }

    //-------------------------------------------------------------------------------------------------

    void CopyGlmToCells(glm::vec3 const & vector, float * cells)
    {
        MFA_ASSERT(cells != nullptr);
        ::memcpy(cells, &vector, 3 * sizeof(float));
    }

    //-------------------------------------------------------------------------------------------------

    void CopyGlmToCells(glm::vec4 const & vector, float * cells)
    {
        MFA_ASSERT(cells != nullptr);
        ::memcpy(cells, &vector, sizeof(float) * 4);
    }

    //-------------------------------------------------------------------------------------------------

    void CopyGlmToCells(glm::quat const & quaternion, float * cells)
    {
        MFA_ASSERT(cells != nullptr);
        ::memcpy(cells, &quaternion, sizeof(float) * 4);
    }

    //-------------------------------------------------------------------------------------------------

    bool IsEqual(glm::mat4 const & matrix, float const * cells)
    {
        MFA_ASSERT(cells != nullptr);
        return ::memcmp(&matrix, cells, 16 * sizeof(float)) == 0;
    }

    //-------------------------------------------------------------------------------------------------

    bool IsEqual(glm::vec2 const & vector, float const * cells)
    {
        MFA_ASSERT(cells != nullptr);
        return ::memcmp(&vector,cells, 2 * sizeof(float)) == 0;
    }

    //-------------------------------------------------------------------------------------------------

    bool IsEqual(glm::vec3 const & vector, float const * cells)
    {
        MFA_ASSERT(cells != nullptr);
        return ::memcmp(&vector,cells, 3 * sizeof(float)) == 0;
    }

    //-------------------------------------------------------------------------------------------------

    bool IsEqual(glm::quat const & quat, float const * cells)
    {
        MFA_ASSERT(cells != nullptr);
        return ::memcmp(&quat,cells, 4 * sizeof(float)) == 0;
    }

    //-------------------------------------------------------------------------------------------------

    bool IsEqual(glm::vec4 const & vector, float const * cells)
    {
        MFA_ASSERT(cells != nullptr);
        return ::memcmp(&vector,cells, 4 * sizeof(float)) == 0;
    }

    //-------------------------------------------------------------------------------------------------

    bool IsEqual(glm::mat4 const & matrixA, glm::mat4 const & matrixB)
    {
        return ::memcmp(&matrixA, &matrixB, 16 * sizeof(float)) == 0;
    }

    //-------------------------------------------------------------------------------------------------

    bool IsEqual(glm::vec2 const & vectorA, glm::vec2 const & vectorB)
    {
        return ::memcmp(&vectorA, &vectorB, 2 * sizeof(float)) == 0;
    }

    //-------------------------------------------------------------------------------------------------

    bool IsEqual(glm::vec3 const & vectorA, glm::vec3 const & vectorB)
    {
        return ::memcmp(&vectorA, &vectorB, 3 * sizeof(float)) == 0;
    }

    //-------------------------------------------------------------------------------------------------

    bool IsEqual(glm::quat const & quatA, glm::quat const & quatB)
    {
        return ::memcmp(&quatA, &quatB, 4 * sizeof(float)) == 0;
    }

    //-------------------------------------------------------------------------------------------------

    bool IsEqual(glm::vec4 const & vectorA, glm::vec4 const & vectorB)
    {
        return ::memcmp(&vectorA, &vectorB, 4 * sizeof(float)) == 0;
    }

    //-------------------------------------------------------------------------------------------------

    glm::quat ToQuat(const float x, const float y, const float z)
    {
        return glm::quat(glm::vec3(
            glm::radians(x),
            glm::radians(y),
            glm::radians(z)
        ));
    }

    //-------------------------------------------------------------------------------------------------

    // Returns degree
    glm::vec3 ToEulerAngles(glm::quat const & quaternion)
    {
        return glm::degrees(glm::eulerAngles(quaternion));
    }

    //-------------------------------------------------------------------------------------------------

    void Rotate(glm::mat4 & transform, float eulerAngles[3])
    {
        transform = glm::rotate(transform, glm::radians(eulerAngles[0]), glm::vec3(1.0f, 0.0f, 0.0f));
        transform = glm::rotate(transform, glm::radians(eulerAngles[1]), glm::vec3(0.0f, 1.0f, 0.0f));
        transform = glm::rotate(transform, glm::radians(eulerAngles[2]), glm::vec3(0.0f, 0.0f, 1.0f));
    }

    //-------------------------------------------------------------------------------------------------

    void Rotate(glm::mat4 & transform, glm::vec3 eulerAngles)
    {
        transform = glm::rotate(transform, glm::radians(eulerAngles[0]), glm::vec3(1.0f, 0.0f, 0.0f));
        transform = glm::rotate(transform, glm::radians(eulerAngles[1]), glm::vec3(0.0f, 1.0f, 0.0f));
        transform = glm::rotate(transform, glm::radians(eulerAngles[2]), glm::vec3(0.0f, 0.0f, 1.0f));
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

}
