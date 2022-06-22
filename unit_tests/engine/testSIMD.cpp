//======================================================================
// 
//======================================================================

#include "catch.hpp"

#include "engine/BedrockMatrix.hpp"

#include <glm/glm.hpp>

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

TEST_CASE("SIMD TestCase3 Lerp", "[SIMD][2]")
{
    vec4 vec1 {1.0f, 2.0f, 5.0f, 1.0f};
    vec4 vec2 {5.0f, 3.0f, 4.0f, 1.0f};
    
    for (float fraction = 0.0f; fraction <= 1.0f; fraction += 0.2f)
    {
        auto const lerpA = Matrix::Lerp(vec1, vec2, fraction);
        auto const lerpB = glm::mix(vec1, vec2, fraction);
        CHECK(lerpA == lerpB);
    }
}

// TODO: Slerp
// TODO: Matrix transform
// TODO: Performance comparasion
