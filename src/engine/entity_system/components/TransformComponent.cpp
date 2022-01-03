#include "TransformComponent.hpp"

#include "engine/BedrockCommon.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/ui_system/UISystem.hpp"
#include "tools/JsonUtils.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    TransformComponent::TransformComponent()
        : mTransform(glm::identity<glm::mat4>())
    {
    }

    //-------------------------------------------------------------------------------------------------

    TransformComponent::TransformComponent(
        glm::vec3 const & position_,
        glm::vec3 const & rotation_,          // In euler angle
        glm::vec3 const & scale_
    )
        : mLocalPosition(position_)
        , mRotation(rotation_)
        , mScale(scale_)
    {}

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::init()
    {
        Component::init();

        auto * parentEntity = GetEntity()->GetParent();
        if (parentEntity != nullptr)
        {
            mParentTransform = parentEntity->GetComponent<TransformComponent>();
            if (auto const parentTransformPtr = mParentTransform.lock())
            {
                mParentTransformChangeListenerId = parentTransformPtr->RegisterChangeListener([this]()-> void
                    {
                        computeTransform();
                    }
                );
            }
        }
        computeTransform();
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::shutdown()
    {
        Component::shutdown();
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
        if (Matrix::IsEqual(mRotation, rotation) == false)
        {
            Matrix::CopyCellsToGlm(rotation, mRotation);
            hasChanged = true;
        }
        if (Matrix::IsEqual(mScale, scale) == false)
        {
            Matrix::CopyCellsToGlm(scale, mScale);
            hasChanged = true;
        }
        if (hasChanged)
        {
            computeTransform();
        }
    }

    void TransformComponent::UpdateTransform(
        glm::vec3 const & position,
        glm::vec3 const & rotation,
        glm::vec3 const & scale
    )
    {
        bool hasChanged = false;
        if (mLocalPosition != position)
        {
            mLocalPosition = position;
            hasChanged = true;
        }
        if (mRotation != rotation)
        {
            mRotation = rotation;
            hasChanged = true;
        }
        if (mScale != scale)
        {
            mScale = scale;
            hasChanged = true;
        }
        if (hasChanged)
        {
            computeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::UpdatePosition(glm::vec3 const & position)
    {
        if (Matrix::IsEqual(mLocalPosition, position) == false)
        {
            mLocalPosition = position;
            computeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::UpdatePosition(float position[3])
    {
        if (Matrix::IsEqual(mLocalPosition, position) == false)
        {
            Matrix::CopyCellsToGlm(position, mLocalPosition);
            computeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::UpdateRotation(glm::vec3 const & rotation)
    {
        if (Matrix::IsEqual(mRotation, rotation) == false)
        {
            mRotation = rotation;
            computeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::UpdateRotation(float rotation[3])
    {
        if (Matrix::IsEqual(mRotation, rotation) == false)
        {
            Matrix::CopyCellsToGlm(rotation, mRotation);
            computeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::UpdateScale(float scale[3])
    {
        if (Matrix::IsEqual(mScale, scale) == false)
        {
            Matrix::CopyCellsToGlm(scale, mScale);
            computeTransform();
        }
    }

    void TransformComponent::UpdateScale(glm::vec3 const & scale)
    {
        if (mScale != scale)
        {
            mScale = scale;
            computeTransform();
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

    void TransformComponent::GetRotation(float outRotation[3]) const
    {
        Matrix::CopyGlmToCells(mRotation, outRotation);
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::GetScale(float outScale[3]) const
    {
        Matrix::CopyGlmToCells(mScale, outScale);
    }

    //-------------------------------------------------------------------------------------------------

    glm::vec3 const & TransformComponent::GetLocalPosition() const
    {
        return mLocalPosition;
    }

    //-------------------------------------------------------------------------------------------------

    glm::vec3 const & TransformComponent::GetRotation() const
    {
        return mRotation;
    }

    //-------------------------------------------------------------------------------------------------

    glm::vec3 const & TransformComponent::GetScale() const
    {
        return mScale;
    }

    //-------------------------------------------------------------------------------------------------

    Signal<>::ListenerId TransformComponent::RegisterChangeListener(std::function<void()> const & listener)
    {
        return mTransformChangeSignal.Register(listener);
    }

    //-------------------------------------------------------------------------------------------------

    bool TransformComponent::UnRegisterChangeListener(Signal<>::ListenerId const listenerId)
    {
        return mTransformChangeSignal.UnRegister(listenerId);
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::onUI()
    {
        if (UI::TreeNode("Transform"))
        {
            Component::onUI();

            glm::vec3 position = mLocalPosition;
            glm::vec3 scale = mScale;
            glm::vec3 rotation = mRotation;

            UI::InputFloat3("Position", position);
            UI::InputFloat3("Scale", scale);
            UI::InputFloat3("Rotation", rotation);

            UpdateTransform(position, rotation, scale);

            UI::TreePop();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::clone(Entity * entity) const
    {
        entity->AddComponent<TransformComponent>(
            mLocalPosition,
            mRotation,
            mScale
        );
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::serialize(nlohmann::json & jsonObject) const
    {
        JsonUtils::SerializeVec3(jsonObject, "position", mLocalPosition);
        JsonUtils::SerializeVec3(jsonObject, "rotation", mRotation);
        JsonUtils::SerializeVec3(jsonObject, "scale", mScale);
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::deserialize(nlohmann::json const & jsonObject)
    {
        JsonUtils::DeserializeVec3(jsonObject, "position", mLocalPosition);
        JsonUtils::DeserializeVec3(jsonObject, "rotation", mRotation);
        JsonUtils::DeserializeVec3(jsonObject, "scale", mScale);
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::computeTransform()
    {
        // Model

        // Position
        auto translateMatrix = glm::identity<glm::mat4>();
        Matrix::Translate(translateMatrix, mLocalPosition);

        // Scale
        auto scaleMatrix = glm::identity<glm::mat4>();
        Matrix::Scale(scaleMatrix, mScale);

        // Rotation
        auto rotationMatrix = glm::identity<glm::mat4>();
        Matrix::Rotate(rotationMatrix, mRotation);

        auto parentTransform = glm::identity<glm::mat4>();
        if (auto const ptr = mParentTransform.lock())
        {
            parentTransform = ptr->GetTransform();
        }

        mTransform = parentTransform * translateMatrix * scaleMatrix * rotationMatrix;

        mWorldPosition = mTransform * glm::vec4 {0 , 0, 0, 1.0f};

        // We notify any class that need to listen to transform component
        mTransformChangeSignal.Emit();

    }

    //-------------------------------------------------------------------------------------------------


}
