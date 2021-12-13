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
        : mPosition(position_)
        , mRotation(rotation_)
        , mScale(scale_)
    {}

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
                        computeTransform();
                    }
                );
            }
        }
        computeTransform();
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
        if (Matrix::IsEqual(mPosition, position) == false)
        {
            Matrix::CopyCellsToGlm(position, mPosition);
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
        if (mPosition != position)
        {
            mPosition = position;
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
        if (Matrix::IsEqual(mPosition, position) == false)
        {
            mPosition = position;
            computeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::UpdatePosition(float position[3])
    {
        if (Matrix::IsEqual(mPosition, position) == false)
        {
            Matrix::CopyCellsToGlm(position, mPosition);
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

    void TransformComponent::GetPosition(float outPosition[3]) const
    {
        Matrix::CopyGlmToCells(mPosition, outPosition);
    }

    //-------------------------------------------------------------------------------------------------

    glm::vec3 TransformComponent::GetAbsolutePosition() const
    {
        // TODO Cache positions
        glm::vec3 position {};

        glm::vec4 const & lastCol = mTransform[3];
        position.x = lastCol.x;
        position.y = lastCol.y;
        position.z = lastCol.z;

        return position;
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

    glm::vec3 const & TransformComponent::GetPosition() const
    {
        return mPosition;
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

    void TransformComponent::OnUI()
    {
        if (UI::TreeNode("Transform"))
        {
            Component::OnUI();

            glm::vec3 position = mPosition;
            glm::vec3 scale = mScale;
            glm::vec3 rotation = mRotation;

            UI::InputFloat3("Position", position.data.data);
            UI::InputFloat3("Scale", scale.data.data);
            UI::InputFloat3("Rotation", rotation.data.data);

            UpdateTransform(position, rotation, scale);

            UI::TreePop();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::Clone(Entity * entity) const
    {
        entity->AddComponent<TransformComponent>(
            mPosition,
            mRotation,
            mScale
        );
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::Serialize(nlohmann::json & jsonObject) const
    {
        JsonUtils::SerializeVec3(jsonObject, "position", mPosition);
        JsonUtils::SerializeVec3(jsonObject, "rotation", mRotation);
        JsonUtils::SerializeVec3(jsonObject, "scale", mScale);
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::Deserialize(nlohmann::json const & jsonObject)
    {
        JsonUtils::DeserializeVec3(jsonObject, "position", mPosition);
        JsonUtils::DeserializeVec3(jsonObject, "rotation", mRotation);
        JsonUtils::DeserializeVec3(jsonObject, "scale", mScale);
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::computeTransform()
    {
        // Model

        // Position
        auto translateMatrix = glm::identity<glm::mat4>();
        Matrix::Translate(translateMatrix, mPosition);

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

        // We notify any class that need to listen to transform component
        mTransformChangeSignal.Emit();

    }

    //-------------------------------------------------------------------------------------------------


}
