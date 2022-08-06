#include "TransformComponent.hpp"

#include "engine/BedrockMatrix.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/ui_system/UI_System.hpp"
#include "tools/JsonUtils.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    TransformComponent::TransformComponent(
        glm::vec3 const & position_,
        glm::vec3 const & rotation_,          // In euler angle
        glm::vec3 const & scale_
    )
        : mLocalPosition(position_)
        , mLocalRotationAngle(rotation_)
        , mLocalScale(scale_)
    {
        mLocalRotationQuat = Matrix::ToQuat(rotation_.x, rotation_.y, rotation_.z);
    }

    
    //-------------------------------------------------------------------------------------------------

    TransformComponent::TransformComponent(
        glm::vec3 const & position_,
        glm::quat const & rotation_,
        glm::vec3 const & scale_
    )
        : mLocalPosition(position_)
        , mLocalRotationQuat(rotation_)
        , mLocalScale(scale_)
    {
        mLocalRotationAngle = Matrix::ToEulerAngles(mLocalRotationQuat);
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::Init()
    {
        Component::Init();

        auto * parentEntity = GetEntity()->GetParent();
        if (parentEntity != nullptr)
        {
            mParentTransform = parentEntity->GetComponent<TransformComponent>();
            if (auto const parentTransformPtr = mParentTransform.lock())
            {
                mParentTransformChangeListenerId = parentTransformPtr->RegisterChangeListener([this]()-> void
                    {
                        ComputeTransform();
                    }
                );
            }
        }
        ComputeTransform();
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::Shutdown()
    {
        Component::Shutdown();
        if (auto const parentTransformPtr = mParentTransform.lock())
        {
            parentTransformPtr->UnRegisterChangeListener(mParentTransformChangeListenerId);
        }
    }
    
    //-------------------------------------------------------------------------------------------------

    void TransformComponent::UpdateLocalTransform(float position[3], float rotation[3], float scale[3])
    {
        bool hasChanged = false;
        if (IsEqual<3>(mLocalPosition, position) == false)
        {
            Copy<3>(mLocalPosition, position);
            hasChanged = true;
        }
        if (IsEqual<3>(mLocalRotationAngle, rotation) == false)
        {
            Copy<3>(mLocalRotationAngle, rotation);
            mLocalRotationQuat = Matrix::ToQuat(mLocalRotationAngle);
            hasChanged = true;
        }
        if (IsEqual<3>(mLocalScale, scale) == false)
        {
            Copy<3>(mLocalScale, scale);
            hasChanged = true;
        }
        if (hasChanged)
        {
            ComputeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::UpdateLocalTransform(
        glm::vec3 const & position,
        glm::vec3 const & rotation,
        glm::vec3 const & scale
    )
    {
        bool hasChanged = false;
        if (IsEqual(mLocalPosition, position) == false)
        {
            Copy(mLocalPosition, position);
            hasChanged = true;
        }
        if (IsEqual(mLocalRotationAngle, rotation) == false)
        {
            Copy(mLocalRotationAngle, rotation);
            mLocalRotationQuat = Matrix::ToQuat(mLocalRotationAngle);
            hasChanged = true;
        }
        if (IsEqual(mLocalScale, scale) == false)
        {
            Copy(mLocalScale, scale);
            hasChanged = true;
        }
        if (hasChanged)
        {
            ComputeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::UpdateLocalPosition(glm::vec3 const & position)
    {
        if (IsEqual(mLocalPosition, position) == false)
        {
            mLocalPosition = position;
            ComputeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::UpdateLocalPosition(float position[3])
    {
        if (IsEqual<3>(mLocalPosition, position) == false)
        {
            Copy<3>(mLocalPosition, position);
            ComputeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::UpdateLocalRotation(glm::vec3 const & rotation)
    {
        if (IsEqual(mLocalRotationAngle, rotation) == false)
        {
            Copy(mLocalRotationAngle, rotation);
            mLocalRotationQuat = Matrix::ToQuat(mLocalRotationAngle);
            ComputeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::UpdateLocalRotation(float rotation[3])
    {
        if (IsEqual<3>(mLocalRotationAngle, rotation) == false)
        {
            Copy<3>(mLocalRotationAngle, rotation);
            mLocalRotationQuat = Matrix::ToQuat(mLocalRotationAngle);
            ComputeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::UpdateLocalScale(float scale[3])
    {
        if (IsEqual<3>(mLocalScale, scale) == false)
        {
            Copy<3>(mLocalScale, scale);
            ComputeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::UpdateLocalScale(glm::vec3 const & scale)
    {
        if (IsEqual(mLocalScale, scale) == false)
        {
            Copy(mLocalScale, scale);
            ComputeTransform();
        }
    }
    
    //-------------------------------------------------------------------------------------------------

    void TransformComponent::UpdateWorldTransform(glm::vec3 const & position, glm::quat const & rotation)
    {
        bool positionChanged = false;
        if (IsEqual(mWorldPosition, position) == false)
        {
            positionChanged = true;
        }

        bool rotationChanged = false;
        if (IsEqual(mWorldRotation, rotation) == false)
        {
            rotationChanged = true;
        }

        if (positionChanged == false && rotationChanged == false)
        {
            return;
        }
        // TODO: Re-check this part
        Copy(mWorldPosition, position);
        Copy(mWorldRotation, rotation);

        auto translateMatrix = glm::identity<glm::mat4>();
        Matrix::Translate(translateMatrix, mWorldPosition);

        // Scale
        auto scaleMatrix = glm::identity<glm::mat4>();
        Matrix::Scale(scaleMatrix, mWorldScale);

        // Rotation
        auto const rotationMatrix = glm::toMat4(mWorldRotation);

        mWorldTransform = translateMatrix * scaleMatrix * rotationMatrix;
        mInverseWorldTransform = glm::inverse(mWorldTransform);

        if (auto const parentTransform = mParentTransform.lock())
        {

            if (positionChanged)
            {
                auto const localTransform = parentTransform->GetInverseWorldTransform() * mWorldTransform;
                mLocalPosition = localTransform * glm::vec4 {0 , 0, 0, 1.0f};
            }

            if (rotationChanged)
            {
                mLocalRotationQuat = glm::inverse(parentTransform->GetWorldRotation()) * mWorldRotation;
                mLocalRotationAngle = Matrix::ToEulerAngles(mLocalRotationQuat);
            }

        }
        else
        {
            if (positionChanged)
            {
                mLocalPosition = position;
            }

            if (rotationChanged)
            {
                mLocalRotationQuat = rotation;
                mLocalRotationAngle = Matrix::ToEulerAngles(mLocalRotationQuat);
            }

            ComputeTransform();
        }

        mTransformChangeSignal.Emit();
        
    }

    //-------------------------------------------------------------------------------------------------

    const glm::mat4 & TransformComponent::GetWorldTransform() const noexcept
    {
        return mWorldTransform;
    }
    
    //-------------------------------------------------------------------------------------------------

    glm::mat4 const & TransformComponent::GetInverseWorldTransform() const noexcept
    {
        return mInverseWorldTransform;
    }

    //-------------------------------------------------------------------------------------------------

    glm::vec4 const & TransformComponent::GetWorldPosition() const
    {
        return mWorldPosition;
    }

    //-------------------------------------------------------------------------------------------------

    glm::vec3 const & TransformComponent::GetLocalPosition() const
    {
        return mLocalPosition;
    }

    //-------------------------------------------------------------------------------------------------

    glm::vec3 const & TransformComponent::GetLocalRotationEulerAngles() const
    {
        return mLocalRotationAngle;
    }

    //-------------------------------------------------------------------------------------------------

    glm::quat const & TransformComponent::GetLocalRotationQuaternion() const
    {
        return mLocalRotationQuat;
    }

    //-------------------------------------------------------------------------------------------------

    glm::quat const & TransformComponent::GetWorldRotation() const
    {
        return mWorldRotation;
    }

    //-------------------------------------------------------------------------------------------------

    glm::vec3 const & TransformComponent::GetLocalScale() const
    {
        return mLocalScale;
    }

    //-------------------------------------------------------------------------------------------------

    glm::vec3 const & TransformComponent::GetWorldScale() const
    {
        return mWorldScale;
    }

    //-------------------------------------------------------------------------------------------------

    SignalId TransformComponent::RegisterChangeListener(std::function<void()> const & listener)
    {
        return mTransformChangeSignal.Register(listener);
    }

    //-------------------------------------------------------------------------------------------------

    bool TransformComponent::UnRegisterChangeListener(SignalId const listenerId)
    {
        return mTransformChangeSignal.UnRegister(listenerId);
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::OnUI()
    {
        if (UI::TreeNode("Transform"))
        {
            Component::OnUI();

            bool changed = false;

            changed |= UI::InputFloat<3>("Position", mLocalPosition);
            changed |= UI::InputFloat<3>("Scale", mLocalScale);
            if (UI::InputFloat<3>("Rotation", mLocalRotationAngle))
            {
                mLocalRotationQuat = Matrix::ToQuat(mLocalRotationAngle);
                changed = true;
            }

            if (changed)
            {
                ComputeTransform();
            }

            UI::TreePop();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::Clone(Entity * entity) const
    {
        entity->AddComponent<TransformComponent>(
            mLocalPosition,
            mLocalRotationAngle,
            mLocalScale
        );
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::Serialize(nlohmann::json & jsonObject) const
    {
        JsonUtils::SerializeVec3(jsonObject, "position", mLocalPosition);
        JsonUtils::SerializeVec3(jsonObject, "rotation", mLocalRotationAngle);
        JsonUtils::SerializeVec3(jsonObject, "scale", mLocalScale);
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::Deserialize(nlohmann::json const & jsonObject)
    {
        JsonUtils::DeserializeVec3(jsonObject, "position", mLocalPosition);
        JsonUtils::DeserializeVec3(jsonObject, "rotation", mLocalRotationAngle);
        JsonUtils::DeserializeVec3(jsonObject, "scale", mLocalScale);
        mLocalRotationQuat = Matrix::ToQuat(mLocalRotationAngle);
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::ComputeTransform()
    {
        // Model

        // Position
        auto translateMatrix = glm::identity<glm::mat4>();
        Matrix::Translate(translateMatrix, mLocalPosition);

        // Scale
        auto scaleMatrix = glm::identity<glm::mat4>();
        Matrix::Scale(scaleMatrix, mLocalScale);

        // Rotation
        auto rotationMatrix = glm::identity<glm::mat4>();
        Matrix::RotateWithEulerAngle(rotationMatrix, mLocalRotationAngle);

        auto pMatrix = glm::identity<glm::mat4>();
        auto pWorldRotation = glm::identity<glm::quat>();
        glm::vec3 pWorldScale {1.0f, 1.0f, 1.0f};
        if (auto const ptr = mParentTransform.lock())
        {
            pMatrix = ptr->GetWorldTransform();
            pWorldRotation = ptr->GetWorldRotation();
            pWorldScale = ptr->GetWorldScale();
        }

        mWorldTransform = pMatrix * translateMatrix * scaleMatrix * rotationMatrix;

        mInverseWorldTransform = glm::inverse(mWorldTransform);

        // TODO: Check this part
        mWorldPosition = mWorldTransform * glm::vec4 { 0, 0, 0, 1.0f };

        mWorldRotation = pWorldRotation * mLocalRotationQuat;

        mWorldScale = pWorldScale * mLocalScale;
        
        // We notify any class that need to listen to transform component
        mTransformChangeSignal.Emit();

    }

    //-------------------------------------------------------------------------------------------------


}
