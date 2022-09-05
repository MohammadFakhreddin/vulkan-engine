#include "TransformComponent.hpp"

#include "engine/BedrockMatrix.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/ui_system/UI_System.hpp"
#include "tools/JsonUtils.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    TransformComponent::TransformComponent(
        glm::vec3 const & localPosition_,
        glm::vec3 const & localEulerAngles_,          // Degrees
        glm::vec3 const & localScale_
    )
        : mLocalPosition(localPosition_)
        , mLocalRotation(localEulerAngles_)
        , mLocalScale(localScale_)
    {}

    
    //-------------------------------------------------------------------------------------------------

    TransformComponent::TransformComponent(
        glm::vec3 const & localPosition_,
        glm::quat const & localQuaternion_,
        glm::vec3 const & localScale_
    )
        : mLocalPosition(localPosition_)
        , mLocalRotation(localQuaternion_)
        , mLocalScale(localScale_)
    {}

    //-------------------------------------------------------------------------------------------------

    TransformComponent::TransformComponent(
        glm::vec3 const& localPosition_,
        Rotation const& localRotation_,
        glm::vec3 const& scale_
    )
        : mLocalPosition(localPosition_)
        , mLocalRotation(localRotation_)
        , mLocalScale(scale_)
    {
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
                mParentTransformChangeListenerId = parentTransformPtr->RegisterChangeListener([this](ChangeParams const & params)-> void
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

    void TransformComponent::SetLocalTransform(float position[3], float eulerAngles[3], float scale[3])
    {
        bool hasChanged = false;
        if (IsEqual<3>(mLocalPosition, position) == false)
        {
            Copy<3>(mLocalPosition, position);
            hasChanged = true;
        }
        if (mLocalRotation != eulerAngles)
        {
            mLocalRotation = eulerAngles;
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

    void TransformComponent::SetLocalTransform(
        glm::vec3 const & position,
        glm::vec3 const & eulerAngles,
        glm::vec3 const & scale
    )
    {
        bool hasChanged = false;
        if (IsEqual(mLocalPosition, position) == false)
        {
            Copy(mLocalPosition, position);
            hasChanged = true;
        }
        if (mLocalRotation != eulerAngles)
        {
            mLocalRotation = eulerAngles;
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

    void TransformComponent::SetLocalTransform(
        glm::vec3 const& position,
        Rotation const& rotation,
        glm::vec3 const& scale
    )
    {
        bool hasChanged = false;
        if (IsEqual(mLocalPosition, position) == false)
        {
            Copy(mLocalPosition, position);
            hasChanged = true;
        }
        if (mLocalRotation != rotation)
        {
            mLocalRotation = rotation;
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

    void TransformComponent::SetLocalPosition(glm::vec3 const & position)
    {
        if (IsEqual(mLocalPosition, position) == false)
        {
            mLocalPosition = position;
            ComputeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::SetLocalPosition(float position[3])
    {
        if (IsEqual<3>(mLocalPosition, position) == false)
        {
            Copy<3>(mLocalPosition, position);
            ComputeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::SetLocalRotation(glm::vec3 const & eulerAngle)
    {
        if (mLocalRotation != eulerAngle)
        {
            mLocalRotation = eulerAngle;
            ComputeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::SetLocalRotation(float rotation[3])
    {
        if (mLocalRotation != rotation)
        {
            mLocalRotation = rotation;
            ComputeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::SetLocalRotation(glm::quat const& quaternion)
    {
        if (mLocalRotation != quaternion)
        {
            mLocalRotation = quaternion;
            ComputeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::SetLocalScale(float scale[3])
    {
        if (IsEqual<3>(mLocalScale, scale) == false)
        {
            Copy<3>(mLocalScale, scale);
            ComputeTransform();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::SetLocalScale(glm::vec3 const & scale)
    {
        if (IsEqual(mLocalScale, scale) == false)
        {
            Copy(mLocalScale, scale);
            ComputeTransform();
        }
    }
    
    //-------------------------------------------------------------------------------------------------

    void TransformComponent::SetWorldTransform(glm::vec3 const & position, glm::quat const & rotation)
    {
        bool positionChanged = false;
        if (IsEqual(mWorldPosition, position) == false)
        {
            positionChanged = true;
            Copy(mWorldPosition, position);
        }

        bool rotationChanged = false;
        if (mWorldRotation != rotation)
        {
            rotationChanged = true;
            mWorldRotation = rotation;
        }

        if (positionChanged == false && rotationChanged == false)
        {
            return;
        }
        
        auto translateMatrix = glm::identity<glm::mat4>();
        Matrix::Translate(translateMatrix, mWorldPosition);

        // Scale
        auto scaleMatrix = glm::identity<glm::mat4>();
        Matrix::Scale(scaleMatrix, mWorldScale);

        // Rotation
        auto const & rotationMatrix = mWorldRotation.GetMatrix();

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
                mLocalRotation = glm::inverse(parentTransform->GetWorldRotation().GetQuaternion()) * mWorldRotation.GetQuaternion();
            }

        }
        else
        {
            if (positionChanged)
            {
                mLocalPosition = mWorldPosition;
            }

            if (rotationChanged)
            {
                mLocalRotation = mWorldRotation;
            }

            ComputeTransform();
        }

        mTransformChangeSignal.Emit(ChangeParams {
            .worldPositionChanged = positionChanged,
            .worldRotationChanged = rotationChanged,
            .worldScaleChanged = false
        });
        
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

    Rotation const & TransformComponent::GetLocalRotation() const
    {
        return mLocalRotation;
    }
    
    //-------------------------------------------------------------------------------------------------

    Rotation const & TransformComponent::GetWorldRotation() const
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

    SignalId TransformComponent::RegisterChangeListener(std::function<void(ChangeParams const &)> const & listener)
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

            changed |= UI::InputFloat("Position", mLocalPosition);
            changed |= UI::InputFloat("Scale", mLocalScale);

            auto eulerAngles = mLocalRotation.GetEulerAngles();
            if (UI::InputFloat("Rotation (Euler angles)", eulerAngles))
            {
                mLocalRotation = eulerAngles;
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
            mLocalRotation,
            mLocalScale
        );
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::Serialize(nlohmann::json & jsonObject) const
    {
        JsonUtils::SerializeVec3(jsonObject, "position", mLocalPosition);
        JsonUtils::SerializeVec3(jsonObject, "rotation", mLocalRotation.GetEulerAngles());
        JsonUtils::SerializeVec3(jsonObject, "scale", mLocalScale);
    }

    //-------------------------------------------------------------------------------------------------

    void TransformComponent::Deserialize(nlohmann::json const & jsonObject)
    {
        JsonUtils::DeserializeVec3(jsonObject, "position", mLocalPosition);

        glm::vec3 eulerAngles{};
        JsonUtils::DeserializeVec3(jsonObject, "rotation", eulerAngles);
        mLocalRotation = eulerAngles;

        JsonUtils::DeserializeVec3(jsonObject, "scale", mLocalScale);
    }

    //-------------------------------------------------------------------------------------------------

    glm::vec3 TransformComponent::Forward() const
    {
        return mWorldRotation.GetMatrix() * Math::ForwardVec4W0;
    }

    //-------------------------------------------------------------------------------------------------

    glm::vec3 TransformComponent::Right() const
    {
        return mWorldRotation.GetMatrix() * Math::RightVec4W0;
    }

    //-------------------------------------------------------------------------------------------------

    glm::vec3 TransformComponent::Up() const
    {
        return mWorldRotation.GetMatrix() * Math::UpVec4W0;
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
        auto const & rotationMatrix = mLocalRotation.GetMatrix();

        auto pMatrix = glm::identity<glm::mat4>();
        Rotation pWorldRotation {};
        glm::vec3 pWorldScale {1.0f, 1.0f, 1.0f};
        if (auto const ptr = mParentTransform.lock())
        {
            pMatrix = ptr->GetWorldTransform();
            pWorldRotation = ptr->GetWorldRotation();
            pWorldScale = ptr->GetWorldScale();
        }

        mWorldTransform = pMatrix * translateMatrix * scaleMatrix * rotationMatrix;

        mInverseWorldTransform = glm::inverse(mWorldTransform);

        auto previousWorldPosition = mWorldPosition;
        mWorldPosition = mWorldTransform * glm::vec4 { 0, 0, 0, 1.0f };

        auto previousWorldRotation = mWorldRotation;
        mWorldRotation = pWorldRotation.GetQuaternion() * mLocalRotation.GetQuaternion();

        auto previousWorldScale = mWorldScale;
        mWorldScale = pWorldScale * mLocalScale;
        
        // We notify any class that need to listen to transform component
        mTransformChangeSignal.Emit(ChangeParams {
            .worldPositionChanged = IsEqual(previousWorldPosition, mWorldPosition) == false,
            .worldRotationChanged = IsEqual(previousWorldRotation, mWorldRotation) == false,
            .worldScaleChanged = IsEqual(previousWorldScale, mWorldScale) == false
        });

    }

    //-------------------------------------------------------------------------------------------------


}
