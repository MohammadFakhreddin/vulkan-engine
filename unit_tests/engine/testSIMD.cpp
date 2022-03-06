//======================================================================
// 
//======================================================================

#include "catch.hpp"

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

