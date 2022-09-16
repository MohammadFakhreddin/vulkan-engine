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
        if (MFA::IsEqual(mEulerAngles, eulerAngles))
        {
            return;
        }
        mEulerAngles = eulerAngles;
        mQuaternion = Matrix::ToQuat(eulerAngles);
        mMatrix = glm::toMat4(mQuaternion);
    }

    //-------------------------------------------------------------------------------------------------

    void Rotation::SetQuaternion(glm::quat const & quaternion)
    {
        if (Matrix::IsEqual(mQuaternion, quaternion))
        {
            return;
        }
        mQuaternion = quaternion;
        mEulerAngles = Matrix::ToEulerAngles(quaternion);
        mMatrix = glm::toMat4(mQuaternion);
    }

    //-------------------------------------------------------------------------------------------------

    bool Rotation::IsEqual(glm::vec3 const & eulerAngles) const
    {
        return MFA::IsEqual(mEulerAngles, eulerAngles);
    }

    //-------------------------------------------------------------------------------------------------

    bool Rotation::IsEqual(glm::quat const & quaternion) const
    {
        return Matrix::IsEqual(mQuaternion, quaternion);
    }

    //-------------------------------------------------------------------------------------------------

    bool Rotation::IsEqual(float eulerAngles[3]) const
    {
        return MFA::IsEqual<3>(mEulerAngles, eulerAngles);
    }

    //-------------------------------------------------------------------------------------------------

    bool Rotation::IsEqual(Rotation const & rotation) const
    {
        return Matrix::IsEqual(mQuaternion, rotation.GetQuaternion());
    }

    //-------------------------------------------------------------------------------------------------

    Rotation & Rotation::Set(float eulerAngles[3])
    {
        Copy<3>(mEulerAngles, eulerAngles);
        mQuaternion = Matrix::ToQuat(mEulerAngles);
        mMatrix = glm::toMat4(mQuaternion);
        return *this;
    }

    //-------------------------------------------------------------------------------------------------

    Rotation & Rotation::Set(glm::vec3 const & eulerAngles)
    {
        Copy(mEulerAngles, eulerAngles);
        mQuaternion = Matrix::ToQuat(mEulerAngles);
        mMatrix = glm::toMat4(mQuaternion);
        return *this;
    }

    //-------------------------------------------------------------------------------------------------

    Rotation & Rotation::Set(glm::quat const & quaternion)
    {
        Copy(mQuaternion, quaternion);
        mEulerAngles = Matrix::ToEulerAngles(quaternion);
        mMatrix = glm::toMat4(mQuaternion);
        return *this;
    }

    //-------------------------------------------------------------------------------------------------

}
