#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>

namespace MFA::Matrix 
{
    glm::mat4 CopyCellsToMat4(float const * cells);

    glm::vec3 CopyCellsToVec3(float const * cells);

    glm::quat CopyCellsToQuat(float const * cells);

    glm::vec4 CopyCellsToVec4(float const * cells);

    void CopyCellsToGlm(float const * cells, glm::mat4 & outMatrix);

    void CopyCellsToGlm(float const * cells, glm::vec3 & outVec3);

    void CopyCellsToGlm(float const * cells, glm::quat & outQuat);

    void CopyCellsToGlm(float const * cells, glm::vec4 & outVec4);
    
    void CopyGlmToCells(glm::mat4 const & matrix, float * cells);

    void CopyGlmToCells(glm::vec2 const & vector, float * cells);
    
    void CopyGlmToCells(glm::vec3 const & vector, float * cells);
    
    void CopyGlmToCells(glm::vec4 const & vector, float * cells);

    void CopyGlmToCells(glm::quat const & quaternion, float * cells);

    bool IsEqual(glm::mat4 const & matrix, float const * cells);

    bool IsEqual(glm::vec2 const & vector, float const * cells);

    bool IsEqual(glm::vec3 const & vector, float const * cells);

    bool IsEqual(glm::quat const & quat, float const * cells);

    bool IsEqual(glm::vec4 const & vector, float const * cells);

    bool IsEqual(glm::mat4 const & matrixA, glm::mat4 const & matrixB);

    bool IsEqual(glm::vec2 const & vectorA, glm::vec2 const & vectorB);

    bool IsEqual(glm::vec3 const & vectorA, glm::vec3 const & vectorB);

    bool IsEqual(glm::quat const & quatA, glm::quat const & quatB);

    bool IsEqual(glm::vec4 const & vectorA, glm::vec4 const & vectorB);


    [[nodiscard]]
    glm::quat ToQuat(float x, float y, float z);

    [[nodiscard]]
    glm::vec3 ToEulerAngles(glm::quat const & quaternion);

    void RotateWithEulerAngle(glm::mat4 & inOutTransform, float eulerAngles[3]);

    void RotateWithEulerAngle(glm::mat4 & inOutTransform, glm::vec3 eulerAngles);

    void RotateWithRadians(glm::mat4 & inOutTransform, float radians[3]);

    void RotateWithRadians(glm::mat4 & inOutTransform, glm::vec3 radians);

    void Scale(glm::mat4 & transform, float scale[3]);

    void Scale(glm::mat4 & transform, glm::vec3 const & scale);

    void Translate(glm::mat4 & transform, float distance[3]);

    void Translate(glm::mat4 & transform, glm::vec3 const & distance);

    void PreparePerspectiveProjectionMatrix(
        float outMatrix[16],
        float aspectRatio,
        float fieldOfView,
        float nearPlane,
        float farPlane 
    );

    void PreparePerspectiveProjectionMatrix(
        glm::mat4 & outMatrix,
        float aspectRatio,
        float fieldOfView,
        float nearPlane,
        float farPlane 
    );

    void PrepareOrthographicProjectionMatrix(
        glm::mat4 & outMatrix,
        float leftPlane,
        float rightPlane,
        float bottomPlane,
        float topPlane,
        float nearPlane,
        float farPlane
    );

}
