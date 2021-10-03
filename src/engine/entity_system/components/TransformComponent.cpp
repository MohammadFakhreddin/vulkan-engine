#include "TransformComponent.hpp"

#include "engine/BedrockCommon.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/entity_system/Entity.hpp"

namespace MFA {

//-------------------------------------------------------------------------------------------------

void TransformComponent::Init()
{
    Component::Init();

    auto * parentEntity = GetEntity()->GetParent();
    if (parentEntity != nullptr)
    {
        mParentTransform = parentEntity->GetComponent<TransformComponent>();
        if (mParentTransform != nullptr)
        {
            mParentTransformChangeListenerId = mParentTransform->RegisterChangeListener([this]()-> void {
                computeTransform();
            });
        }
    }
    computeTransform();
}

//-------------------------------------------------------------------------------------------------

void TransformComponent::Shutdown()
{
    Component::Shutdown();
    mParentTransform->UnRegisterChangeListener(mParentTransformChangeListenerId);
}

//-------------------------------------------------------------------------------------------------

void TransformComponent::UpdateTransform(float position[3], float rotation[3], float scale[3])
{
    bool hasChanged = false;
    if (IsEqual<3>(mPosition, position) == false) {
        Copy<3>(mPosition, position);
        hasChanged = true;
    }
    if (IsEqual<3>(mRotation, rotation) == false) {
        Copy<3>(mRotation, rotation);
        hasChanged = true;
    }
    if (IsEqual<3>(mScale, scale) == false) {
        Copy<3>(mScale, scale);
        hasChanged = true;
    }
    if (hasChanged) {
        computeTransform();
    }
}

//-------------------------------------------------------------------------------------------------

void TransformComponent::UpdatePosition(float position[3]) {
    bool hasChanged = false;
    if (IsEqual<3>(mPosition, position) == false) {
        Copy<3>(mPosition, position);
        hasChanged = true;
    }
    if (hasChanged) {
        computeTransform();
    }
}

//-------------------------------------------------------------------------------------------------

void TransformComponent::UpdateRotation(float rotation[3]) {
    bool hasChanged = false;
    if (IsEqual<3>(mRotation, rotation) == false) {
        Copy<3>(mRotation, rotation);
        hasChanged = true;
    }
    if (hasChanged) {
        computeTransform();
    }
}

//-------------------------------------------------------------------------------------------------

void TransformComponent::UpdateScale(float scale[3]) {
    bool hasChanged = false;
    if (IsEqual<3>(mScale, scale) == false) {
        Copy<3>(mScale, scale);
        hasChanged = true;
    }
    if (hasChanged) {
        computeTransform();
    }
}

//-------------------------------------------------------------------------------------------------

const glm::mat4 & TransformComponent::GetTransform() const noexcept {
    return mTransform;
}

//-------------------------------------------------------------------------------------------------

void TransformComponent::GetPosition(float outPosition[3]) const {
    Copy<3>(outPosition, mPosition);
}

//-------------------------------------------------------------------------------------------------

void TransformComponent::GetRotation(float outRotation[3]) const {
    Copy<3>(outRotation, mRotation);
}

//-------------------------------------------------------------------------------------------------

void TransformComponent::GetScale(float outScale[3]) const {
    Copy<3>(outScale, mScale);
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

void TransformComponent::computeTransform() {
    // Model

    // Position
    auto translateMatrix = glm::identity<glm::mat4>();
    Matrix::GlmTranslate(translateMatrix, mPosition);

    // Scale
    auto scaleMatrix = glm::identity<glm::mat4>();
    Matrix::GlmScale(scaleMatrix, mScale);
    
    // Rotation
    auto rotationMatrix = glm::identity<glm::mat4>();
    Matrix::GlmRotate(rotationMatrix, mRotation);

    auto parentTransform = glm::identity<glm::mat4>();
    if (mParentTransform != nullptr)
    {
        parentTransform = mParentTransform->GetTransform();
    }

    mTransform = parentTransform * translateMatrix * scaleMatrix * rotationMatrix;

    // We notify any class that need to listen to transform component
    mTransformChangeSignal.Emit();
    
}

//-------------------------------------------------------------------------------------------------


}
