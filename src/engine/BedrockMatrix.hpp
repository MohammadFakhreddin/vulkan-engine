#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

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


    [[nodiscard]]
    glm::quat ToQuat(float x, float y, float z);

    [[nodiscard]]
    glm::vec3 ToEulerAngles(glm::quat const & quaternion);

    void Rotate(glm::mat4 & transform, float eulerAngles[3]);

    void Rotate(glm::mat4 & transform, glm::vec3 eulerAngles);

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

}
