//======================================================================
// 
//======================================================================

#include "catch.hpp"

#include "engine/BedrockMatrix.hpp"
#include "engine/BedrockLog.hpp"

#include <glm/glm.hpp>

#include <immintrin.h>

using namespace MFA;
using namespace glm;

//======================================================================

TEST_CASE("SIMD TestCase1", "[SIMD][0]") {
    auto const a = vec4 {1.0, 2.0, 3.0, 4.0};
    auto const b = vec4 {1.0, 2.0, 3.0, 4.0};
    auto const c = Matrix::Add(a, b);
    CHECK(c[0] == 2.0);
    CHECK(c[1] == 4.0);
    CHECK(c[2] == 6.0);
    CHECK(c[3] == 8.0);
}

TEST_CASE("SIMD TestCase2 DotProduct", "[SIMD][1]")
{
    vec4 vec1 {1.0f, 2.0f, 5.0f, 1.0f};
    vec4 vec2 {5.0f, 3.0f, 4.0f, 1.0f};
    CHECK(Matrix::Dot(vec1, vec2) == dot(vec1, vec2));
}

template <
    class result_t   = std::chrono::milliseconds,
    class clock_t    = std::chrono::steady_clock,
    class duration_t = std::chrono::milliseconds
>
auto since(std::chrono::time_point<clock_t, duration_t> const& start)
{
    return std::chrono::duration_cast<result_t>(clock_t::now() - start);
}

TEST_CASE("SIMD TestCase3 Lerp", "[SIMD][2]")
{
    vec4 vec1 {1.0f, 2.0f, 5.0f, 1.0f};
    vec4 vec2 {5.0f, 3.0f, 4.0f, 1.0f};
    
    uint64_t stepCount = 10000001;
    float stepValue = 1.0f / static_cast<float>(stepCount - 1);

    std::vector<glm::vec4> glmResults (stepCount);
    std::vector<glm::vec4> simdResults (stepCount);

    {// GLM
        auto startTime = std::chrono::steady_clock::now();
        
        uint64_t currStep = 0;
        for (float fraction = 0.0f; fraction <= 1.0f; fraction += stepValue)
        {
            glmResults[currStep] = glm::mix(vec1, vec2, fraction);
            ++currStep;
        }

        MFA_LOG_INFO("GLM Elapsed(ms)=%d", since(startTime).count());
    }

    {// SIMD
        auto startTime = std::chrono::steady_clock::now();
        
        int currStep = 0;
        for (float fraction = 0.0f; fraction <= 1.0f; fraction += stepValue)
        {
            simdResults[currStep] = Matrix::Lerp(vec1, vec2, fraction);
            ++currStep;
        }

        MFA_LOG_INFO("SIMD Elapsed(ms)=%d", since(startTime).count());
    }

    {// Correctness check
        for (uint64_t i = 0; i < stepCount; ++i)
        {
            CHECK(glmResults[i] == simdResults[i]);
        }
    }
}

// TODO: Slerp
// TODO: Matrix transform
// TODO: Performance comparasion
