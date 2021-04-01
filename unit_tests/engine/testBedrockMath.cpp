//======================================================================
// 
//======================================================================

#include "catch.hpp"

#include "engine/BedrockMatrix.hpp"

//======================================================================
//----------------------------------------------------------------------

//======================================================================
TEST_CASE("Matrix TestCase1", "[Matrix][0]") {
    using namespace MFA;
    Matrix4X4Float a {};
    a.set(0, 0, 5);
    a.set(0, 1, 7);
    a.set(0, 2, 9);
    a.set(0, 3, 10);

    a.set(1, 0, 2);
    a.set(1, 1, 3);
    a.set(1, 2, 3);
    a.set(1, 3, 8);

    a.set(2, 0, 8);
    a.set(2, 1, 10);
    a.set(2, 2, 2);
    a.set(2, 3, 3);

    a.set(3, 0, 3);
    a.set(3, 1, 3);
    a.set(3, 2, 4);
    a.set(3, 3, 8);

    a.print();

    Matrix4X4Float b {};
    b.set(0, 0, 3);
    b.set(0, 1, 10);
    b.set(0, 2, 12);
    b.set(0, 3, 18);

    b.set(1, 0, 12);
    b.set(1, 1, 1);
    b.set(1, 2, 4);
    b.set(1, 3, 9);

    b.set(2, 0, 9);
    b.set(2, 1, 10);
    b.set(2, 2, 12);
    b.set(2, 3, 2);

    b.set(3, 0, 3);
    b.set(3, 1, 12);
    b.set(3, 2, 4);
    b.set(3, 3, 10);

    b.print();

    a.multiply(b);

    a.print();

    CHECK(a.get(0, 0) == 210);
    CHECK(a.get(0, 1) == 267);
    CHECK(a.get(0, 2) == 236);
    CHECK(a.get(0, 3) == 271);

    CHECK(a.get(1, 0) == 93);
    CHECK(a.get(1, 1) == 149);
    CHECK(a.get(1, 2) == 104);
    CHECK(a.get(1, 3) == 149);

    CHECK(a.get(2, 0) == 171);
    CHECK(a.get(2, 1) == 146);
    CHECK(a.get(2, 2) == 172);
    CHECK(a.get(2, 3) == 268);

    CHECK(a.get(3, 0) == 105);
    CHECK(a.get(3, 1) == 169);
    CHECK(a.get(3, 2) == 128);
    CHECK(a.get(3, 3) == 169);

} 