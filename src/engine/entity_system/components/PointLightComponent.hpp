#pragma once

#include "ColorComponent.hpp"
#include "TransformComponent.hpp"
#include "engine/entity_system/Component.hpp"

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include "AxisAlignedBoundingBoxComponent.hpp"

namespace MFA {

    class MeshRendererComponent;
    class ColorComponent;
    class TransformComponent;

    class PointLightComponent final : public Component
    {
    public:

        MFA_COMPONENT_PROPS(
            PointLightComponent,
            FamilyType::PointLight,
            EventTypes::InitEvent | EventTypes::ShutdownEvent,
            Component
        )

        explicit PointLightComponent();
        explicit PointLightComponent(
            float radius,
            float maxDistance,
            std::weak_ptr<MeshRendererComponent> attachedMesh = {}         // Optional: Use this parameter only if you want the light to be visible if a certain mesh is visible
        );

        ~PointLightComponent() override;

        void Init() override;

        void Shutdown() override;

        [[nodiscard]]
        glm::vec3 GetLightColor() const;

        [[nodiscard]]
        glm::vec3 GetPosition() const;

        [[nodiscard]]
        float GetRadius() const;

        void OnUI() override;
        
        [[nodiscard]]
        bool IsVisible() const;

        bool IsBoundingVolumeInRange(BoundingVolumeComponent const * bvComponent) const;

        void GetShadowViewProjectionMatrices(float outData[6][16]) const;

        [[nodiscard]]
        float GetMaxDistance() const;

        [[nodiscard]]
        float GetMaxSquareDistance() const;

        [[nodiscard]]
        float GetLinearAttenuation() const;

        [[nodiscard]]
        float GetQuadraticAttenuation() const;

        void Clone(Entity * entity) const override;

        void Serialize(nlohmann::json & jsonObject) const override;

        void Deserialize(nlohmann::json const & jsonObject) override;

    private:

        // We only need to compute projection once
        void computeProjection();

        void computeAttenuation();

        /*
        We have to compute viewProjection every time that transform position changes
        Future idea: We can mark as dirty and do the computation only when we are visible
        */
        void computeViewProjectionMatrices();

        std::weak_ptr<ColorComponent> mColorComponent {};
        std::weak_ptr<TransformComponent> mTransformComponent {};

        float mRadius = 0.0f;                                       // Radius is used for light attenuation
        float mMaxDistance = 0.0f;                                  // Max distance that light should support. Used for optimization
        float mProjectionNearDistance = 0.0f;                       // Used for shadow projection
        float mProjectionFarDistance = 0.0f;                        // Used for shadow projection

        float mMaxSquareDistance = 0.0f;
        float mLinearAttenuation = 0.0f;
        float mQuadraticAttenuation = 0.0f;

        std::weak_ptr<MeshRendererComponent> mAttachedMesh {};     // Used for visibility check

        glm::mat4 mShadowProjectionMatrix {};

        float mShadowViewProjectionMatrices[6][16] {};             // For shadow computation

        int mTransformChangeListenerId = 0;

    };

    using PointLight = PointLightComponent;

}
