#include "BedrockMatrix.hpp"

#include "BedrockAssert.hpp"
#include "BedrockMath.hpp"

namespace MFA::Matrix {

//-------------------------------------------------------------------------------------------------

glm::mat4 CopyCellsToMat4(float const * cells) {
    MFA_ASSERT(cells != nullptr);
    glm::mat4 result {};
    CopyCellsToMat4(cells, result);
    return result;
}

//-------------------------------------------------------------------------------------------------

glm::vec3 ConvertCellsToVec3(float const * cells) {
    MFA_ASSERT(cells != nullptr);
    return glm::vec3 {cells[0], cells[1], cells[2]};
}

//-------------------------------------------------------------------------------------------------

glm::quat ConvertCellsToQuat(float const * cells) {
    MFA_ASSERT(cells != nullptr);
    glm::quat result {};
    result.x = cells[0];
    result.y = cells[1];
    result.z = cells[2];
    result.w = cells[3];
    return result;
}

//-------------------------------------------------------------------------------------------------

glm::vec4 ConvertCellsToVec4(float const * cells) {
    MFA_ASSERT(cells != nullptr);
    return glm::vec4 {cells[0], cells[1], cells[2], cells[3]};
}

//-------------------------------------------------------------------------------------------------

void CopyCellsToMat4(float const * cells, glm::mat4 & outMatrix) {
    ::memcpy(&outMatrix, cells, sizeof(float) * 16);
}

//-------------------------------------------------------------------------------------------------

void CopyGlmToCells(glm::mat4 const & matrix, float * cells) {
    ::memcpy(cells, &matrix, sizeof(float) * 16);
}

//-------------------------------------------------------------------------------------------------

void CopyGlmToCells(glm::vec2 const & matrix, float * cells) {
    cells[0] = matrix[0];
    cells[1] = matrix[1];
}

//-------------------------------------------------------------------------------------------------

void CopyGlmToCells(glm::vec3 const & matrix, float * cells) {
    cells[0] = matrix[0];
    cells[1] = matrix[1];
    cells[2] = matrix[2];
}

//-------------------------------------------------------------------------------------------------

void CopyGlmToCells(glm::vec4 const & matrix, float * cells) {
    cells[0] = matrix[0];
    cells[1] = matrix[1];
    cells[2] = matrix[2];
    cells[3] = matrix[3];
}

//-------------------------------------------------------------------------------------------------

bool IsEqual(glm::mat4 const & matrix, float const * cells) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (cells[i * 4 + j] != matrix[i][j]) {
                return false;
            }
        }
    }
    return true;
}

//-------------------------------------------------------------------------------------------------

glm::quat GlmToQuat(const float x, const float y, const float z) {
    return glm::quat(glm::vec3(
        glm::radians(x),
        glm::radians(y),
        glm::radians(z)
    ));   
}

//-------------------------------------------------------------------------------------------------

// Returns degree
glm::vec3 GlmToEulerAngles(glm::quat const & quaternion) {
    return glm::degrees(glm::eulerAngles(quaternion));
}

//-------------------------------------------------------------------------------------------------

void GlmRotate(glm::mat4 & transform, float eulerAngles[3]) {
    transform = glm::rotate(transform, glm::radians(eulerAngles[0]), glm::vec3(1.0f, 0.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(eulerAngles[1]), glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(eulerAngles[2]), glm::vec3(0.0f, 0.0f, 1.0f));
}

//-------------------------------------------------------------------------------------------------

void GlmScale(glm::mat4 & transform, float scale[3]) {
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

void GlmTranslate(glm::mat4 & transform, float distance[3]) {
    transform = glm::translate(transform, glm::vec3(distance[0], distance[1], distance[2]));
}

//-------------------------------------------------------------------------------------------------

void PreparePerspectiveProjectionMatrix(
    float outMatrix[16], 
    float const aspectRatio, 
    float const fieldOfView, 
    float const nearPlane, 
    float const farPlane
) {
    float const invTan = 1.0f / tan(Math::Deg2Rad( 0.5f * fieldOfView ));
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
) {
    float const invTan = 1.0f / tan(Math::Deg2Rad( 0.5f * fieldOfView ));
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
