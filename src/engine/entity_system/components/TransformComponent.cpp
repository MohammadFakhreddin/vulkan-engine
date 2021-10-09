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
    if (Matrix::IsEqual(mPosition, position) == false) {
        Matrix::CopyCellsToGlm(position, mPosition);
        hasChanged = true;
    }
    if (Matrix::IsEqual(mRotation, rotation) == false) {
        Matrix::CopyCellsToGlm(rotation, mRotation);
        hasChanged = true;
    }
    if (Matrix::IsEqual(mScale, scale) == false) {
        Matrix::CopyCellsToGlm(scale, mScale);
        hasChanged = true;
    }
    if (hasChanged) {
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
    if (mPosition != position) {
        mPosition = position;
        hasChanged = true;
    }
    if (mRotation != rotation) {
        mRotation = rotation;
        hasChanged = true;
    }
    if (mScale != scale) {
        mScale = scale;
        hasChanged = true;
    }
    if (hasChanged) {
        computeTransform();
    }
}

//-------------------------------------------------------------------------------------------------

void TransformComponent::UpdatePosition(float position[3]) {
    if (Matrix::IsEqual(mPosition, position) == false) {
        Matrix::CopyCellsToGlm(position, mPosition);
        computeTransform();
    }
}

//-------------------------------------------------------------------------------------------------

void TransformComponent::UpdateRotation(float rotation[3]) {
    if (Matrix::IsEqual(mRotation, rotation) == false) {
        Matrix::CopyCellsToGlm(rotation, mRotation);
        computeTransform();
    }
}

//-------------------------------------------------------------------------------------------------

void TransformComponent::UpdateScale(float scale[3]) {
    if (Matrix::IsEqual(mScale, scale) == false) {
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

const glm::mat4 & TransformComponent::GetTransform() const noexcept {
    return mTransform;
}

//-------------------------------------------------------------------------------------------------

void TransformComponent::GetPosition(float outPosition[3]) const {
    Matrix::CopyGlmToCells(mPosition, outPosition);
}

//-------------------------------------------------------------------------------------------------

void TransformComponent::GetRotation(float outRotation[3]) const {
    Matrix::CopyGlmToCells(mRotation, outRotation);
}

//-------------------------------------------------------------------------------------------------

void TransformComponent::GetScale(float outScale[3]) const {
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

void TransformComponent::computeTransform() {
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
