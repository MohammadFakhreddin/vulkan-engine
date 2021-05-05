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
    Matrix4X4Float::identity(a);

    Matrix4X4Float b {};
    Matrix4X4Float::AssignTranslation(b, 10, 10, 10);

    a.multiply(b);

    CHECK(a.equal(b));

}

TEST_CASE("Matrix TestCase3", "[Matrix][2]") {
    using namespace MFA;

    Matrix4X1Float const point {1, 1, 1, 0};
    // TODO Fix this function
    Matrix3X1Float const eulerAnglesInRad {
        Math::Deg2Rad<float>(45.0f),
        Math::Deg2Rad<float>(270.0f),
        Math::Deg2Rad<float>(90.0f)
    };
    //eulerAnglesInRad.print("eulerAnglesInRad");

    Matrix4X4Float eulerAngleMat {};
    Matrix4X4Float::AssignRotation(
        eulerAngleMat,
        eulerAnglesInRad
    );

    Matrix4X1Float eulerAnglePoint {};
    eulerAnglePoint.assign(point);
    eulerAnglePoint.multiply(eulerAngleMat);

    //eulerAnglePoint.print("eulerAnglePoint");

    //eulerAngleMat.print("eulerAngleMat");

    const QuaternionFloat quaternion = Matrix3X1Float::ToQuaternion(eulerAnglesInRad);

    Matrix3X1Float convertedEulerAngles = Matrix4X1Float::QuaternionToEulerAnglesRadian(quaternion);
    //convertedEulerAngles.print("ConvertedQuaternion");
    CHECK(Matrix3X1Float::Equal(eulerAnglesInRad, convertedEulerAngles));

    Matrix4X4Float quaternionMat {};
    Matrix4X4Float::AssignRotation(quaternionMat, quaternion);

    Matrix4X1Float quaternionPoint {};
    quaternionPoint.assign(point);
    quaternionPoint.multiply(quaternionMat);

    //quaternionMat.print("quaternionMat");

    //quaternionPoint.print("quaternionPoint");

    CHECK(Matrix4X1Float::Equal(quaternionPoint, eulerAnglePoint));
}