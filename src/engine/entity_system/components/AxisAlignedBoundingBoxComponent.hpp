#pragma once

#include "BoundingVolumeComponent.hpp"

namespace MFA
{

    class PBR_Essence;
    class TransformComponent;

    class AxisAlignedBoundingBoxComponent final : public BoundingVolumeComponent
    {
    public:

        MFA_COMPONENT_PROPS(
            AxisAlignedBoundingBoxComponent,
            FamilyType::BoundingVolume,
            EventTypes::InitEvent | EventTypes::ShutdownEvent | EventTypes::UpdateEvent
        )
        
        explicit AxisAlignedBoundingBoxComponent(
            glm::vec3 const & center,
            glm::vec3 const & extend,
            bool occlusionCullingEnabled
        );

        // Only used for auto deserialization
        explicit AxisAlignedBoundingBoxComponent();

        ~AxisAlignedBoundingBoxComponent() override;
        
        void Init() override;

        void Shutdown() override;

        void onUI() override;

        void clone(Entity * entity) const override;

        void deserialize(nlohmann::json const & jsonObject) override;

        void serialize(nlohmann::json & jsonObject) const override;

        [[nodiscard]]
        glm::vec3 const & GetExtend() const override;

        [[nodiscard]]
        glm::vec3 const & GetLocalPosition() const override;

        [[nodiscard]]
        float GetRadius() const override;

        [[nodiscard]]
        glm::vec4 const & GetWorldPosition() const override;

    protected:

        bool IsInsideCameraFrustum(CameraComponent const * camera) override;

        void computeWorldPosition();

    private:

        //bool mIsAutoGenerated;

        glm::vec3 mLocalPosition {};
        glm::vec3 mExtend {};               // Idea: We can apply object scale on extend so it always have correct values
        float mRadius = 0.0f;

        glm::vec4 mWorldPosition {};

        glm::vec3 mAABB_Extent {};
        
        std::weak_ptr<TransformComponent> mTransformComponent {};

        int mTransformChangeListener = -1;
    
    };

    using AABB = AxisAlignedBoundingBoxComponent;

}
