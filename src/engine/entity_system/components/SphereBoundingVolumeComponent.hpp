#pragma once

#include "BoundingVolumeComponent.hpp"

namespace MFA {

class TransformComponent;

class SphereBoundingVolumeComponent final : public BoundingVolumeComponent
{
public:

    MFA_COMPONENT_PROPS(
        SphereBoundingVolumeComponent,
        FamilyType::BoundingVolume,
        EventTypes::UpdateEvent | EventTypes::InitEvent
    )

    explicit SphereBoundingVolumeComponent();
    ~SphereBoundingVolumeComponent() override;

    explicit SphereBoundingVolumeComponent(float radius);
    
    void init() override;

    [[nodiscard]]
    glm::vec3 const & GetExtend() const override;

    [[nodiscard]]
    glm::vec3 const & GetLocalPosition() const override;

    [[nodiscard]]
    float GetRadius() const override;

    [[nodiscard]]
    glm::vec4 const & GetWorldPosition() const override;

    void clone(Entity * entity) const override;

    void serialize(nlohmann::json & jsonObject) const override;

    void deserialize(nlohmann::json const & jsonObject) override;

protected:

    bool IsInsideCameraFrustum(CameraComponent const * camera) override;

private:

    float mRadius = 0.0f;
    std::weak_ptr<TransformComponent> mTransformComponent {};

};

}
