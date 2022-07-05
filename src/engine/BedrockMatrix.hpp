#pragma once

#include <foundation/PxVec3.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>

#ifdef ENABLE_SIMD
#include <immintrin.h>
#endif

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
    glm::quat ToQuat(float xDeg, float yDeg, float zDeg);

    [[nodiscard]]
    glm::quat ToQuat(glm::vec3 const & eulerAngles);

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

    // Physx (TODO: Is this the right place for this class ?)
    void CopyCellsToPhysx(float const * cells, physx::PxVec3 & outVec3);

    void CopyCellsToPhysx(float const * cells, physx::PxQuat & outQuat);

    void CopyGlmToPhysx(glm::vec3 const & inVec3, physx::PxVec3 & outVec3);

    void CopyGlmToPhysx(glm::quat const & inQuat, physx::PxQuat & outQuat);

    physx::PxVec3 CopyGlmToPhysx(glm::vec3 const & inVec3);

    physx::PxQuat CopyGlmToPhysx(glm::quat const & inQuat);

    physx::PxTransform CopyGlmToPhysx(
        glm::vec3 const & position,
        glm::quat const & rotation
    );


#ifdef ENABLE_SIMD

    static __m256 convertToM256(glm::vec4 const & vec)
    {
        return _mm256_set_ps(vec[0], vec[1], vec[2], vec[3], 0.0f, 0.0f, 0.0f, 0.0f);
    }

    static glm::vec4 convertToVec4(__m256 const & var)
    {
        auto const * values = reinterpret_cast<float const *>(&var);
        return glm::vec4 {values[7], values[6], values[5], values[4]};
    }

    [[nodiscard]]
    glm::vec4 Add(glm::vec4 const & vec1, glm::vec4 const & vec2)
    {
        __m256 a = convertToM256(vec1);
        __m256 b = convertToM256(vec2);
        __m256 c = _mm256_add_ps(a, b);
        return convertToVec4(c);
    }

    [[nodiscard]]
    float Dot(glm::vec4 const & vec1, glm::vec4 const & vec2)
    {
        __m256 const a = convertToM256(vec1);
        __m256 const b = convertToM256(vec2);
        auto const c = _mm256_dp_ps(a, b, 0b11111000);
        auto const * values = reinterpret_cast<float const *>(&c);
        return values[7];
    }

    [[nodiscard]]
    glm::vec4 Lerp(glm::vec4 const & vec1, glm::vec4 const & vec2, float fraction)
    {
        __m256 aVar = convertToM256(vec1);
        __m256 const aFrac = _mm256_set1_ps(1.0f - fraction);
        aVar = _mm256_mul_ps(aVar, aFrac);

        __m256 bVar = convertToM256(vec2);
        __m256 const bFrac = _mm256_set1_ps(fraction);
        bVar = _mm256_mul_ps(bVar, bFrac);

        __m256 cVar = _mm256_add_ps(aVar, bVar);
        return convertToVec4(cVar);
    }

#endif

}
