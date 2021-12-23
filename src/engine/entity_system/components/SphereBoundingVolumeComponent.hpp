#pragma once

#include "BoundingVolumeComponent.hpp"

namespace MFA {

class TransformComponent;

class SphereBoundingVolumeComponent final : public BoundingVolumeComponent
{
public:

    MFA_COMPONENT_PROPS(
        SphereBoundingVolumeComponent,
        FamilyType::BoundingVolumeComponent,
        EventTypes::UpdateEvent | EventTypes::InitEvent
    )

    explicit SphereBoundingVolumeComponent();
    ~SphereBoundingVolumeComponent() override;

    explicit SphereBoundingVolumeComponent(float radius);
    
    void Init() override;

    [[nodiscard]]
    glm::vec3 const & GetExtend() const override;

    [[nodiscard]]
    glm::vec3 const & GetLocalPosition() const override;

    [[nodiscard]]
    float GetRadius() const override;

    [[nodiscard]]
    glm::vec4 const & GetWorldPosition() const override;

    void Clone(Entity * entity) const override;

    void Serialize(nlohmann::json & jsonObject) const override;

    void Deserialize(nlohmann::json const & jsonObject) override;

protected:

    bool IsInsideCameraFrustum(CameraComponent const * camera) override;

private:

    float mRadius = 0.0f;
    std::weak_ptr<TransformComponent> mTransformComponent {};

};

}
