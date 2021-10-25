#include "AxisAlignedBoundingBoxComponent.hpp"

#include "MeshRendererComponent.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"
#include "engine/render_system/drawable_essence/DrawableEssence.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/camera/CameraComponent.hpp"
#include "engine/ui_system/UISystem.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    AxisAlignedBoundingBoxComponent::AxisAlignedBoundingBoxComponent(
        glm::vec3 const & center,
        glm::vec3 const & extend
    )
        : mIsAutoGenerated(false)
        , mCenter(center)
        , mExtend(extend)
    {}

    //-------------------------------------------------------------------------------------------------

    AxisAlignedBoundingBoxComponent::AxisAlignedBoundingBoxComponent()
        : mIsAutoGenerated(true)
    {}

    //-------------------------------------------------------------------------------------------------

    void AxisAlignedBoundingBoxComponent::Init()
    {
        BoundingVolumeComponent::Init();

        mTransformComponent = GetEntity()->GetComponent<TransformComponent>();
        MFA_ASSERT(mTransformComponent.expired() == false);

        if (mIsAutoGenerated)
        {
            auto const meshRendererComponent = GetEntity()->GetComponent<MeshRendererComponent>().lock();
            MFA_ASSERT(meshRendererComponent != nullptr);

            auto * essence = meshRendererComponent->GetVariant().lock()->GetEssence();
            MFA_ASSERT(essence != nullptr);

            auto const & mesh = essence->GetGpuModel().model.mesh;
            MFA_ASSERT(mesh.HasPositionMinMax());

            auto * positionMax = mesh.GetPositionMax();
            auto * positionMin = mesh.GetPositionMin();

            mExtend.x = abs(positionMax[0] - positionMin[0]);
            mExtend.y = abs(positionMax[1] - positionMin[1]);
            mExtend.z = abs(positionMax[2] - positionMin[2]);

            mCenter.x = (positionMax[0] + positionMin[0]) / 2.0f;
            mCenter.y = (positionMax[1] + positionMin[1]) / 2.0f;
            mCenter.z = (positionMax[2] + positionMin[2]) / 2.0f;

        }
    }

    //-------------------------------------------------------------------------------------------------

    auto AxisAlignedBoundingBoxComponent::DEBUG_GetCenterAndRadius() -> DEBUG_CenterAndRadius
    {
        DEBUG_CenterAndRadius const result{
            .center = mCenter,
            .extend = mExtend
        };

        return result;
    }

    //-------------------------------------------------------------------------------------------------

    void AxisAlignedBoundingBoxComponent::OnUI()
    {
        if (UI::TreeNode("AxisAlignedBoundingBox"))
        {
            BoundingVolumeComponent::OnUI();
            UI::InputFloat3("Center", mCenter.data.data);
            UI::InputFloat3("Extend", mExtend.data.data);
            UI::TreePop();
        }
    }

    //-------------------------------------------------------------------------------------------------

    bool AxisAlignedBoundingBoxComponent::IsInsideCameraFrustum(CameraComponent const * camera)
    {
        MFA_ASSERT(camera != nullptr);

        auto const transformComponentPtr = mTransformComponent.lock();
        if (transformComponentPtr == nullptr)
        {
            return false;
        }

        //Get global scale thanks to our transform
        const glm::vec3 globalCenter{ transformComponentPtr->GetTransform() * glm::vec4(mCenter, 1.f) };

        // Scaled orientation
        auto forwardDirection = CameraComponent::ForwardVector;
        forwardDirection = transformComponentPtr->GetTransform() * forwardDirection;
        forwardDirection = glm::normalize(forwardDirection);
        forwardDirection *= mExtend.z;

        auto rightDirection = CameraComponent::RightVector;
        rightDirection = transformComponentPtr->GetTransform() * rightDirection;
        rightDirection = glm::normalize(rightDirection);
        rightDirection *= mExtend.x;

        auto upDirection = CameraComponent::UpVector;
        upDirection = transformComponentPtr->GetTransform() * upDirection;
        upDirection = glm::normalize(upDirection);
        upDirection *= mExtend.y;

        glm::vec3 const right = rightDirection;
        glm::vec3 const up = upDirection;
        glm::vec3 const forward = forwardDirection;

        const float newIi = std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, right)) +
            std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, up)) +
            std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, forward));

        const float newIj = std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, right)) +
            std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, up)) +
            std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, forward));

        const float newIk = std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, right)) +
            std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, up)) +
            std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, forward));

        return camera->IsPointInsideFrustum(globalCenter, glm::vec3(newIi, newIj, newIk));
    }

    //-------------------------------------------------------------------------------------------------

}
