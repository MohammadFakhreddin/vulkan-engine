#include "AxisAlignedBoundingBoxComponent.hpp"

#include "MeshRendererComponent.hpp"
#include "engine/BedrockMath.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/asset_system/AssetModel.hpp"
#include "engine/camera/CameraComponent.hpp"
#include "engine/ui_system/UI_System.hpp"
#include "tools/JsonUtils.hpp"
#include "engine/resource_manager/ResourceManager.hpp"
#include "engine/entity_system/components/RendererComponent.hpp"
#include "engine/render_system/pipelines/EssenceBase.hpp"
#include "engine/render_system/pipelines/VariantBase.hpp"
#include "engine/asset_system/Asset_PBR_Mesh.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    AxisAlignedBoundingBoxComponent::AxisAlignedBoundingBoxComponent(
        glm::vec3 const & center,
        glm::vec3 const & extend
    )
        : mIsAutoGenerated(false)
        , mLocalPosition(center)
        , mExtend(extend)
    {}

    //-------------------------------------------------------------------------------------------------

    AxisAlignedBoundingBoxComponent::AxisAlignedBoundingBoxComponent()
        : mIsAutoGenerated(true)
    {}

    //-------------------------------------------------------------------------------------------------

    AxisAlignedBoundingBoxComponent::~AxisAlignedBoundingBoxComponent() = default;

    //-------------------------------------------------------------------------------------------------

    void AxisAlignedBoundingBoxComponent::init()
    {
        BoundingVolumeComponent::init();

        mTransformComponent = GetEntity()->GetComponent<TransformComponent>();
        MFA_ASSERT(mTransformComponent.expired() == false);
        if (auto const transformComponent = mTransformComponent.lock())
        {
            mTransformChangeListener = transformComponent->RegisterChangeListener([this]()->void {computeWorldPosition();});
        }

        if (mIsAutoGenerated)
        {
            auto const rendererComponent = GetEntity()->GetComponent<MeshRendererComponent>().lock();
            MFA_ASSERT(rendererComponent != nullptr);

            auto * variant = rendererComponent->getVariant();
            MFA_ASSERT(variant != nullptr);
            auto * essence = variant->GetEssence();
            MFA_ASSERT(essence != nullptr);
            auto * gpuModel = essence->getGpuModel();
            MFA_ASSERT(gpuModel != nullptr);

            auto const model = RC::AcquireCpuModel(gpuModel->nameOrAddress.c_str());
            MFA_ASSERT(model != nullptr);

            auto const mesh = model->mesh;
            MFA_ASSERT(mesh != nullptr);

            auto const & meshData = static_cast<AS::PBR::Mesh *>(mesh.get())->getMeshData();
            MFA_ASSERT(meshData->hasPositionMinMax);
            auto * positionMax = meshData->positionMax;
            auto * positionMin = meshData->positionMin;
            MFA_ASSERT(positionMax != nullptr);
            MFA_ASSERT(positionMin != nullptr);

            mExtend.x = abs(positionMax[0] - positionMin[0]);
            mExtend.y = abs(positionMax[1] - positionMin[1]);
            mExtend.z = abs(positionMax[2] - positionMin[2]);

            mLocalPosition.x = (positionMax[0] + positionMin[0]) / 2.0f;
            mLocalPosition.y = (positionMax[1] + positionMin[1]) / 2.0f;
            mLocalPosition.z = (positionMax[2] + positionMin[2]) / 2.0f;

        }

        mRadius = Math::Max(mExtend.x, Math::Max(mExtend.y, mExtend.z)) / 2.0f;
        MFA_ASSERT(mRadius > 0);

        computeWorldPosition();
    }

    //-------------------------------------------------------------------------------------------------

    void AxisAlignedBoundingBoxComponent::shutdown()
    {
        if (auto const transformComponent = mTransformComponent.lock())
        {
            transformComponent->UnRegisterChangeListener(mTransformChangeListener);
        }
    }

    //-------------------------------------------------------------------------------------------------

    void AxisAlignedBoundingBoxComponent::onUI()
    {
        if (UI::TreeNode("AxisAlignedBoundingBox"))
        {
            glm::vec3 localPosition = mLocalPosition;
            
            BoundingVolumeComponent::onUI();
            UI::InputFloat3("Center", localPosition);
            UI::InputFloat3("Extend", mExtend);
            UI::TreePop();

            if (mLocalPosition != localPosition)
            {
                mLocalPosition = localPosition;
                computeWorldPosition();
            }
        }
    }

    //-------------------------------------------------------------------------------------------------

    void AxisAlignedBoundingBoxComponent::clone(Entity * entity) const
    {
        MFA_ASSERT(entity != nullptr);
        entity->AddComponent<AxisAlignedBoundingBoxComponent>(
            mLocalPosition,
            mExtend
        );
    }

    //-------------------------------------------------------------------------------------------------

    void AxisAlignedBoundingBoxComponent::deserialize(nlohmann::json const & jsonObject)
    {
        JsonUtils::DeserializeVec3(jsonObject, "center", mLocalPosition);
        JsonUtils::DeserializeVec3(jsonObject, "extend", mExtend);
        mIsAutoGenerated = false;
    }

    //-------------------------------------------------------------------------------------------------

    void AxisAlignedBoundingBoxComponent::serialize(nlohmann::json & jsonObject) const
    {
        JsonUtils::SerializeVec3(jsonObject, "center", mLocalPosition);
        JsonUtils::SerializeVec3(jsonObject, "extend", mExtend);
    }

    //-------------------------------------------------------------------------------------------------

    glm::vec3 const & AxisAlignedBoundingBoxComponent::GetExtend() const
    {
        return mExtend;
    }

    //-------------------------------------------------------------------------------------------------

    glm::vec3 const & AxisAlignedBoundingBoxComponent::GetLocalPosition() const
    {
        return mLocalPosition;
    }

    //-------------------------------------------------------------------------------------------------

    float AxisAlignedBoundingBoxComponent::GetRadius() const
    {
        return mRadius;
    }

    //-------------------------------------------------------------------------------------------------

    glm::vec4 const & AxisAlignedBoundingBoxComponent::GetWorldPosition() const
    {
        return mWorldPosition;
    }

    //-------------------------------------------------------------------------------------------------

    bool AxisAlignedBoundingBoxComponent::IsInsideCameraFrustum(CameraComponent const * camera)
    {
        MFA_ASSERT(camera != nullptr);

        const float newIi = std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, mRightVector)) +
            std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, mUpVector)) +
            std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, mForwardVector));

        const float newIj = std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, mRightVector)) +
            std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, mUpVector)) +
            std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, mForwardVector));

        const float newIk = std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, mRightVector)) +
            std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, mUpVector)) +
            std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, mForwardVector));

        return camera->IsPointInsideFrustum(mWorldPosition, glm::vec3(newIi, newIj, newIk));
    }

    //-------------------------------------------------------------------------------------------------

    void AxisAlignedBoundingBoxComponent::computeWorldPosition()
    {
        auto const transformComponent = mTransformComponent.lock();
        if (transformComponent == nullptr) {
            return;
        }

        auto const & transform = transformComponent->GetTransform();

        mWorldPosition = transform * glm::vec4(mLocalPosition, 1.f);

        // Scaled orientation
        auto forwardDirection = Math::ForwardVector4;
        forwardDirection = transform * forwardDirection;
        forwardDirection = glm::normalize(forwardDirection);
        forwardDirection *= mExtend.z;
        mForwardVector = forwardDirection;

        auto rightDirection = Math::RightVector4;
        rightDirection = transform * rightDirection;
        rightDirection = glm::normalize(rightDirection);
        rightDirection *= mExtend.x;
        mRightVector = rightDirection;

        auto upDirection = Math::UpVector4;
        upDirection = transform * upDirection;
        upDirection = glm::normalize(upDirection);
        upDirection *= mExtend.y;
        mUpVector = upDirection;
    }

    //-------------------------------------------------------------------------------------------------

}
