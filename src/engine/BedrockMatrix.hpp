#pragma once

#include "engine/BedrockMath.hpp"

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/norm.hpp>

#ifdef ENABLE_SIMD
#include <immintrin.h>
#endif

namespace MFA::Matrix 
{

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

    template<typename T>
    bool IsNearZero(T const & glmVec) {
        return glm::length2(glmVec) < Math::Epsilon<float>();
    }

    glm::quat ToRotation(glm::vec3 direction);

    // Returns angle of 2 quaternions in radians
    [[nodiscard]]
    float Angle(glm::quat const & a, glm::quat const & b);

    [[nodiscard]]
    bool IsEqualUsingDot(float const dot);

    [[nodiscard]]
    bool IsEqual(glm::quat const & a, glm::quat const & b);

    /***
     *
     * @param from Quaternion that we start from
     * @param to Quaternion that we want to reach
     * @param maxDegreeDelta Max degrees in radian that we can move
     ***/
    glm::quat RotateTowards(
        glm::quat const & from,
        glm::quat const & to,
        float maxDegreeDelta
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
