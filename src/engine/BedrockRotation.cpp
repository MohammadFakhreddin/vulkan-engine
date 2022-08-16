#include "BedrockRotation.hpp"

#include "engine/BedrockMatrix.hpp"
#include "engine/BedrockCommon.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    Rotation::Rotation() = default;

    //-------------------------------------------------------------------------------------------------

    Rotation::Rotation(glm::vec3 const & eulerAngles)
        : mQuaternion(Matrix::ToQuat(eulerAngles))
        , mEulerAngles(eulerAngles)
        , mMatrix(glm::toMat4(mQuaternion))
    {}

    //-------------------------------------------------------------------------------------------------

    Rotation::Rotation(glm::quat const & quaternion)
        : mQuaternion(quaternion)
        , mEulerAngles(Matrix::ToEulerAngles(quaternion))
        , mMatrix(glm::toMat4(quaternion))
    {}

    //-------------------------------------------------------------------------------------------------

    glm::vec3 const & Rotation::GetEulerAngles() const
    {
        return mEulerAngles;
    }

    //-------------------------------------------------------------------------------------------------

    glm::quat const & Rotation::GetQuaternion() const
    {
        return mQuaternion;
    }

    //-------------------------------------------------------------------------------------------------

    glm::mat4 const & Rotation::GetMatrix() const
    {
        return mMatrix;
    }

    //-------------------------------------------------------------------------------------------------

    void Rotation::SetEulerAngles(glm::vec3 const & eulerAngles)
    {
        mEulerAngles = eulerAngles;
        mQuaternion = Matrix::ToQuat(eulerAngles);
        mMatrix = glm::toMat4(mQuaternion);
    }

    //-------------------------------------------------------------------------------------------------

    void Rotation::SetQuaternion(glm::quat const & quaternion)
    {
        mQuaternion = quaternion;
        mEulerAngles = Matrix::ToEulerAngles(quaternion);
        mMatrix = glm::toMat4(mQuaternion);
    }

    //-------------------------------------------------------------------------------------------------

    bool Rotation::operator==(glm::vec3 const & eulerAngles) const
    {
        return IsEqual(mEulerAngles, eulerAngles);
    }

    //-------------------------------------------------------------------------------------------------

    bool Rotation::operator==(glm::quat const & quaternion) const
    {
        return IsEqual(mQuaternion, quaternion);
    }

    //-------------------------------------------------------------------------------------------------

    bool Rotation::operator==(float eulerAngles[3]) const
    {
        return IsEqual<3>(mEulerAngles, eulerAngles);
    }

    //-------------------------------------------------------------------------------------------------

    bool Rotation::operator==(Rotation const & rotation) const
    {
        return IsEqual(mQuaternion, rotation.GetQuaternion());
    }

    //-------------------------------------------------------------------------------------------------

    Rotation & Rotation::operator=(float eulerAngles[3])
    {
        Copy<3>(mEulerAngles, eulerAngles);
        mQuaternion = Matrix::ToQuat(mEulerAngles);
        mMatrix = glm::toMat4(mQuaternion);
        return *this;
    }

    //-------------------------------------------------------------------------------------------------

    Rotation & Rotation::operator=(glm::vec3 const & eulerAngles)
    {
        Copy(mEulerAngles, eulerAngles);
        mQuaternion = Matrix::ToQuat(mEulerAngles);
        mMatrix = glm::toMat4(mQuaternion);
        return *this;
    }

    //-------------------------------------------------------------------------------------------------

    Rotation & Rotation::operator=(glm::quat const & quaternion)
    {
        Copy(mQuaternion, quaternion);
        mEulerAngles = Matrix::ToEulerAngles(quaternion);
        mMatrix = glm::toMat4(mQuaternion);
        return *this;
    }

    //-------------------------------------------------------------------------------------------------

}
