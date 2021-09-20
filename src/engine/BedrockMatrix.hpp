#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace MFA::Matrix 
{
    glm::mat4 CopyCellsToMat4(float const * cells);

    glm::vec3 ConvertCellsToVec3(float const * cells);

    glm::quat ConvertCellsToQuat(float const * cells);

    glm::vec4 ConvertCellsToVec4(float const * cells);

    void CopyCellsToMat4(float const * cells, glm::mat4 & outMatrix);

    void CopyGlmToCells(glm::mat4 const & matrix, float * cells);

    void CopyGlmToCells(glm::vec2 const & matrix, float * cells);
    
    void CopyGlmToCells(glm::vec3 const & matrix, float * cells);
    
    void CopyGlmToCells(glm::vec4 const & matrix, float * cells);

    bool IsEqual(glm::mat4 const & matrix, float const * cells);

    [[nodiscard]]
    glm::quat GlmToQuat(float x, float y, float z);

    void GlmRotate(glm::mat4 & transform, float eulerAngles[3]);

    void GlmScale(glm::mat4 & transform, float scale[3]);

    void GlmTranslate(glm::mat4 & transform, float distance[3]);

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
