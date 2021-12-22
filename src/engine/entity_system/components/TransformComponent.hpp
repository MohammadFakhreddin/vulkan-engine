#pragma once

#include "engine/entity_system/Component.hpp"
#include "engine/BedrockSignal.hpp"

#include <mat4x4.hpp>

namespace MFA
{

    class TransformComponent final : public Component
    {
    public:

        MFA_COMPONENT_PROPS(
            TransformComponent,
            FamilyType::TransformComponent,
            EventTypes::InitEvent | EventTypes::ShutdownEvent
        )
        
        explicit TransformComponent();
        explicit TransformComponent(
            glm::vec3 const & position_,
            glm::vec3 const & rotation_,          // In euler angle
            glm::vec3 const & scale_
        );

        void Init() override;

        void Shutdown() override;

        void UpdateTransform(float position[3], float rotation[3], float scale[3]);

        void UpdateTransform(glm::vec3 const & position, glm::vec3 const & rotation, glm::vec3 const & scale);

        void UpdatePosition(glm::vec3 const & position);

        void UpdatePosition(float position[3]);

        void UpdateRotation(glm::vec3 const & rotation);

        void UpdateRotation(float rotation[3]);

        void UpdateScale(float scale[3]);
        
        void UpdateScale(glm::vec3 const & scale);

        [[nodiscard]]
        glm::mat4 const & GetTransform() const noexcept;

        void GetLocalPosition(float outPosition[3]) const;

        [[nodiscard]]
        glm::vec4 const & GetWorldPosition() const;

        void GetRotation(float outRotation[3]) const;

        void GetScale(float outScale[3]) const;

        [[nodiscard]]
        glm::vec3 const & GetLocalPosition() const;

        [[nodiscard]]
        glm::vec3 const & GetRotation() const;

        [[nodiscard]]
        glm::vec3 const & GetScale() const;

        Signal<>::ListenerId RegisterChangeListener(std::function<void()> const & listener);

        bool UnRegisterChangeListener(Signal<>::ListenerId listenerId);

        void OnUI() override;

        void Clone(Entity * entity) const override;

        void Serialize(nlohmann::json & jsonObject) const override;

        void Deserialize(nlohmann::json const & jsonObject) override;

    private:

        void computeTransform();

        Signal<> mTransformChangeSignal {};

        glm::vec3 mLocalPosition { 0.0f, 0.0f, 0.0f };
        glm::vec3 mRotation { 0.0f, 0.0f, 0.0f };          // In euler angle // TODO Use Quaternion instead! Soon! ToQuat is heavy
        glm::vec3 mScale { 1.0f, 1.0f, 1.0f };

        glm::vec4 mWorldPosition {};

        glm::mat4 mTransform;

        std::weak_ptr<TransformComponent> mParentTransform {};

        Signal<>::ListenerId mParentTransformChangeListenerId {};
    };

}
