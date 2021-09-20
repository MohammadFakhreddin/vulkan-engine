//======================================================================
// 
//======================================================================

#include "catch.hpp"

#include "engine/BedrockMatrix.hpp"
#include "engine/camera/CameraBase.h"

//======================================================================
//----------------------------------------------------------------------

//======================================================================
TEST_CASE("Matrix TestCase1", "[Matrix][0]") {
    using namespace MFA;
    Matrix4X4Float a {};
    a.set(0, 0, 5.0f);
    a.set(0, 1, 7.0f);
    a.set(0, 2, 9.0f);
    a.set(0, 3, 10.0f);

    a.set(1, 0, 2.0f);
    a.set(1, 1, 3.0f);
    a.set(1, 2, 3.0f);
    a.set(1, 3, 8.0f);

    a.set(2, 0, 8.0f);
    a.set(2, 1, 10.0f);
    a.set(2, 2, 2.0f);
    a.set(2, 3, 3.0f);

    a.set(3, 0, 3.0f);
    a.set(3, 1, 3.0f);
    a.set(3, 2, 4.0f);
    a.set(3, 3, 8.0f);

    a.print();

    Matrix4X4Float b {};
    b.set(0, 0, 3.0f);
    b.set(0, 1, 10.0f);
    b.set(0, 2, 12.0f);
    b.set(0, 3, 18.0f);

    b.set(1, 0, 12.0f);
    b.set(1, 1, 1.0f);
    b.set(1, 2, 4.0f);
    b.set(1, 3, 9.0f);

    b.set(2, 0, 9.0f);
    b.set(2, 1, 10.0f);
    b.set(2, 2, 12.0f);
    b.set(2, 3, 2.0f);

    b.set(3, 0, 3.0f);
    b.set(3, 1, 12.0f);
    b.set(3, 2, 4.0f);
    b.set(3, 3, 10.0f);

    b.print();

    a.multiply(b);

    a.print();

    CHECK(a.get(0, 0) == 210.0f);
    CHECK(a.get(0, 1) == 267.0f);
    CHECK(a.get(0, 2) == 236.0f);
    CHECK(a.get(0, 3) == 271.0f);

    CHECK(a.get(1, 0) == 93.0f);
    CHECK(a.get(1, 1) == 149.0f);
    CHECK(a.get(1, 2) == 104.0f);
    CHECK(a.get(1, 3) == 149.0f);

    CHECK(a.get(2, 0) == 171.0f);
    CHECK(a.get(2, 1) == 146.0f);
    CHECK(a.get(2, 2) == 172.0f);
    CHECK(a.get(2, 3) == 268.0f);

    CHECK(a.get(3, 0) == 105.0f);
    CHECK(a.get(3, 1) == 169.0f);
    CHECK(a.get(3, 2) == 128.0f);
    CHECK(a.get(3, 3) == 169.0f);

}

TEST_CASE("Matrix TestCase2", "[Matrix][1]") {
    using namespace MFA;

    Matrix4X4Float a {};
    Matrix4X4Float::Identity(a);

    Matrix4X4Float b {};
    Matrix4X4Float::AssignTranslation(b, 10, 10, 10);

    a.multiply(b);

    CHECK(a.equal(b));

}

TEST_CASE("Matrix TestCase3", "[Matrix][2]") {
    using namespace MFA;

    Matrix3X3Float a {};
    a.set(0, 0, 1);
    a.set(0, 1, 0);
    a.set(0, 2, 2);

    a.set(1, 0, 2);
    a.set(1, 1, 1);
    a.set(1, 2, 3);

    a.set(2, 0, 1);
    a.set(2, 1, 0);
    a.set(2, 2, 4);

    _Matrix<float, 3, 1> b {2.0f, 6.0f, 1.0f};
    
    b.multiply(a);

    Matrix3X1Float c {15.0f, 6.0f, 26.0f};

    CHECK(b.equal(c));
}

//TEST_CASE("Matrix TestCase4", "[Matrix][3]") {
//    using namespace MFA;
//
//    Matrix3X3Float a {};
//    a.set(0, 0, 1);
//    a.set(0, 1, 0);
//    a.set(0, 2, 2);
//
//    a.set(1, 0, 2);
//    a.set(1, 1, 1);
//    a.set(1, 2, 3);
//
//    a.set(2, 0, 1);
//    a.set(2, 1, 0);
//    a.set(2, 2, 4);
//
//    _Matrix<float, 3,2> b {};
//    b.set(0, 0, 2);
//    b.set(0, 1, 5);
//    b.set(1, 0, 6);
//    b.set(1, 1, 7);
//    b.set(2, 0, 1);
//    b.set(2, 1, 8);
//    
//    b.multiply(a);
//
//    _Matrix<float, 3,2> c {};
//    c.set(0, 0, 15);
//    c.set(0, 1, 27);
//    c.set(1, 0, 6);
//    c.set(1, 1, 7);   
//    c.set(2, 0, 26);
//    c.set(2, 1, 63);
//
//    CHECK(b.equal(c));
//}
