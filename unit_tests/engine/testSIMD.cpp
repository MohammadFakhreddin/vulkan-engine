//======================================================================
// 
//======================================================================

#include "catch.hpp"

#include <glm/glm.hpp>

#include <immintrin.h>

//======================================================================

TEST_CASE("SIMD TestCase1", "[SIMD][0]") {

    // Construction from scalars or literals.
    __m256d a = _mm256_set_pd(1.0, 2.0, 3.0, 4.0);

    // Does GCC generate the correct mov, or (better yet) elide the copy
    // and pass two of the same register into the add? Let's look at the assembly.
    __m256d b = a;

    // Add the two vectors, interpreting the bits as 4 double-precision
    // floats.
    __m256d c = _mm256_add_pd(a, b);
    
    double const * values = reinterpret_cast<double *>(&c);

    printf("%f %f %f %f\n", values[3], values[2], values[1], values[0]);
    CHECK(values[3] == 2.0);
    CHECK(values[2] == 4.0);
    CHECK(values[1] == 6.0);
    CHECK(values[0] == 8.0);
}

TEST_CASE("SIMD TestCase2 DotProduct", "[SIMD][1]")
{
    glm::vec4 vec1 {1.0f, 2.0f, 5.0f, 1.0f};
    glm::vec4 vec2 {5.0f, 3.0f, 4.0f, 1.0f};
    __m256 const a = _mm256_set_ps(vec1[0], vec1[1], vec1[2], vec1[3], 0.0f, 0.0f, 0.0f, 0.0f);
    __m256 const b = _mm256_set_ps(vec2[0], vec2[1], vec2[2], vec2[3], 0.0f, 0.0f, 0.0f, 0.0f);
    auto const dotProd = _mm256_dp_ps(a, b, 0b11111000);
    auto const * dotProdElems = reinterpret_cast<float const *>(&dotProd);
    CHECK(dotProdElems[7] == glm::dot(vec1, vec2));
}

