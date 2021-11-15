#pragma once

#include "ColorComponent.hpp"
#include "TransformComponent.hpp"
#include "engine/entity_system/Component.hpp"
#include "engine/entity_system/Entity.hpp"

#include <vec3.hpp>
#include <mat4x4.hpp>

namespace MFA {

    class DrawableVariant;
    class ColorComponent;
    class TransformComponent;

    class PointLightComponent final : public Component
    {
    public:

        MFA_COMPONENT_PROPS(PointLightComponent)
        MFA_COMPONENT_CLASS_TYPE_1(ClassType::PointLightComponent)
        MFA_COMPONENT_REQUIRED_EVENTS(EventTypes::InitEvent | EventTypes::ShutdownEvent)

        explicit PointLightComponent(
            float radius,
            float maxDistance,
            float projectionNearDistance,
            float projectionFarDistance,
            std::weak_ptr<DrawableVariant> attachedVariant = {}         // Optional: Use this parameter only if you want the light to be visible if a certain mesh is visible
        );

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

        void GetShadowViewProjectionMatrices(float outData[6][16]) const;

        [[nodiscard]]
        float GetMaxDistance() const;

        [[nodiscard]]
        float GetMaxSquareDistance() const;

        [[nodiscard]]
        float GetLinearAttenuation() const;

        [[nodiscard]]
        float GetQuadraticAttenuation() const;

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
        float const mProjectionNearDistance;                        // Used for shadow projection
        float const mProjectionFarDistance;                         // Used for shadow projection

        float mMaxSquareDistance = 0.0f;
        float mLinearAttenuation = 0.0f;
        float mQuadraticAttenuation = 0.0f;

        std::weak_ptr<DrawableVariant> mDrawableVariant {};     // Used for visibility check

        glm::mat4 mShadowProjectionMatrix {};

        float mShadowViewProjectionMatrices[6][16] {};          // For shadow computation

        int mTransformChangeListenerId = 0;

    };

}
