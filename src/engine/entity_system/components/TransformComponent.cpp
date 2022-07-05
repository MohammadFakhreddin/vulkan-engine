#include "TransformComponent.hpp"

#include "engine/BedrockMatrix.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/ui_system/UI_System.hpp"
#include "tools/JsonUtils.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    TransformComponent::TransformComponent()
        : mTransform(glm::identity<glm::mat4>())
    {
    }

    //-------------------------------------------------------------------------------------------------
    // TODO: We should use angles only for editor
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

    void TransformComponent::UpdateTransform(float position[3], float rotation[3], float scale[3])
    {
        bool hasChanged = false;
        if (Matrix::IsEqual(mLocalPosition, position) == false)
        {
            Matrix::CopyCellsToGlm(position, mLocalPosition);
            hasChanged = true;
        }
        if (Matrix::IsEqual(mLocalRotationAngle, rotation) == false)
        {
            Matrix::CopyCellsToGlm(rotation, mLocalRotationAngle);
            hasChanged = true;
        }
        if (Matrix::IsEqual(mLocalScale, scale) == false)
        {
            Matrix::CopyCellsToGlm(scale, mLocalScale);
            hasChanged = true;
        }
        if (hasChanged)
        {
            ComputeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::UpdateTransform(
        glm::vec3 const & position,
        glm::vec3 const & rotation,
        glm::vec3 const & scale
    )
    {
        bool hasChanged = false;
        if (Matrix::IsEqual(mLocalPosition, position) == false)
        {
            mLocalPosition = position;
            hasChanged = true;
        }
        if (Matrix::IsEqual(mLocalRotationAngle, rotation) == false)
        {
            mLocalRotationAngle = rotation;
            hasChanged = true;
        }
        if (Matrix::IsEqual(mLocalScale, scale) == false)
        {
            mLocalScale = scale;
            hasChanged = true;
        }
        if (hasChanged)
        {
            ComputeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::UpdatePosition(glm::vec3 const & position)
    {
        if (Matrix::IsEqual(mLocalPosition, position) == false)
        {
            mLocalPosition = position;
            ComputeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::UpdatePosition(float position[3])
    {
        if (Matrix::IsEqual(mLocalPosition, position) == false)
        {
            Matrix::CopyCellsToGlm(position, mLocalPosition);
            ComputeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::UpdateRotation(glm::vec3 const & rotation)
    {
        if (Matrix::IsEqual(mLocalRotationAngle, rotation) == false)
        {
            mLocalRotationAngle = rotation;
            ComputeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::UpdateRotation(float rotation[3])
    {
        if (Matrix::IsEqual(mLocalRotationAngle, rotation) == false)
        {
            Matrix::CopyCellsToGlm(rotation, mLocalRotationAngle);
            ComputeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::UpdateScale(float scale[3])
    {
        if (Matrix::IsEqual(mLocalScale, scale) == false)
        {
            Matrix::CopyCellsToGlm(scale, mLocalScale);
            ComputeTransform();
        }
    }

    void TransformComponent::UpdateScale(glm::vec3 const & scale)
    {
        if (mLocalScale != scale)
        {
            mLocalScale = scale;
            ComputeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    const glm::mat4 & TransformComponent::GetTransform() const noexcept
    {
        return mTransform;
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::GetLocalPosition(float outPosition[3]) const
    {
        Matrix::CopyGlmToCells(mLocalPosition, outPosition);
    }

    //-------------------------------------------------------------------------------------------------

    glm::vec4 const & TransformComponent::GetWorldPosition() const
    {
        return mWorldPosition;
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::GetLocalRotation(float outRotation[3]) const
    {
        Matrix::CopyGlmToCells(mLocalRotationAngle, outRotation);
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::GetLocalScale(float outScale[3]) const
    {
        Matrix::CopyGlmToCells(mLocalScale, outScale);
    }

    //-------------------------------------------------------------------------------------------------

    glm::vec3 const & TransformComponent::GetLocalPosition() const
    {
        return mLocalPosition;
    }

    //-------------------------------------------------------------------------------------------------

    glm::vec3 const & TransformComponent::GetLocalRotation() const
    {
        return mLocalRotationAngle;
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

            glm::vec3 position = mLocalPosition;
            glm::vec3 scale = mLocalScale;
            glm::vec3 rotation = mLocalRotationAngle;

            UI::InputFloat3("Position", position);
            UI::InputFloat3("Scale", scale);
            UI::InputFloat3("Rotation", rotation);

            UpdateTransform(position, rotation, scale);

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
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::ComputeTransform()
    {
        mLocalRotationQuat = Matrix::ToQuat(mLocalRotationAngle);
    
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
        if (auto const ptr = mParentTransform.lock())
        {
            pMatrix = ptr->GetTransform();
            pWorldRotation = ptr->GetWorldRotation();
        }

        mTransform = pMatrix * translateMatrix * scaleMatrix * rotationMatrix;

        // TODO: Check this part
        mWorldPosition = mTransform * glm::vec4 {0 , 0, 0, 1.0f};

        mWorldRotation = pWorldRotation * mLocalRotationQuat;
        
        // We notify any class that need to listen to transform component
        mTransformChangeSignal.Emit();

    }

    //-------------------------------------------------------------------------------------------------


}
