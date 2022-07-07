#pragma once

#include "engine/entity_system/Component.hpp"
#include "engine/BedrockSignal.hpp"

#include <glm/mat4x4.hpp>
#include <glm/gtx/quaternion.hpp>

namespace MFA
{

    class TransformComponent final : public Component
    {
    public:

        MFA_COMPONENT_PROPS(
            TransformComponent,
            FamilyType::Transform,
            EventTypes::InitEvent | EventTypes::ShutdownEvent,
            Component
        )
        
        explicit TransformComponent();
        explicit TransformComponent(
            glm::vec3 const & position_,
            glm::vec3 const & rotation_,          // In euler angle
            glm::vec3 const & scale_
        );
        explicit TransformComponent(
            glm::vec3 const & position_,
            glm::quat const & rotation_,          // In euler angle
            glm::vec3 const & scale_
        );

        void Init() override;

        void Shutdown() override;

        void UpdateLocalTransform(float position[3], float rotation[3], float scale[3]);

        void UpdateLocalTransform(glm::vec3 const & position, glm::vec3 const & rotation, glm::vec3 const & scale);

        void UpdateLocalPosition(glm::vec3 const & position);

        void UpdateLocalPosition(float position[3]);

        void UpdateLocalRotation(glm::vec3 const & rotation);

        void UpdateLocalRotation(float rotation[3]);

        void UpdateLocalScale(float scale[3]);
        
        void UpdateLocalScale(glm::vec3 const & scale);

        void UpdateWorldTransform(
            glm::vec3 const & position,
            glm::quat const & rotation
        );

        [[nodiscard]]
        glm::mat4 const & GetWorldTransform() const noexcept;

        [[nodiscard]]
        glm::mat4 const & GetInverseWorldTransform() const noexcept;

        [[nodiscard]]
        glm::vec4 const & GetWorldPosition() const;

        [[nodiscard]]
        glm::vec3 const & GetLocalPosition() const;

        [[nodiscard]]
        glm::vec3 const & GetLocalRotationEulerAngles() const;

        [[nodiscard]]
        glm::quat const & GetLocalRotationQuaternion() const;

        [[nodiscard]]
        glm::quat const & GetWorldRotation() const;

        [[nodiscard]]
        glm::vec3 const & GetLocalScale() const;

        glm::vec3 const & GetWorldScale() const;

        SignalId RegisterChangeListener(std::function<void()> const & listener);

        bool UnRegisterChangeListener(SignalId listenerId);

        void OnUI() override;

        void Clone(Entity * entity) const override;

        void Serialize(nlohmann::json & jsonObject) const override;

        void Deserialize(nlohmann::json const & jsonObject) override;

    private:

        void ComputeTransform();

        Signal<> mTransformChangeSignal {};

        glm::vec3 mLocalPosition { 0.0f, 0.0f, 0.0f };
        glm::vec3 mLocalRotationAngle { 0.0f, 0.0f, 0.0f };          // In euler angle // TODO Use Quaternion instead! Soon! ToQuat is heavy
        glm::quat mLocalRotationQuat {};
        glm::vec3 mLocalScale { 1.0f, 1.0f, 1.0f };

        glm::vec4 mWorldPosition {};
        glm::quat mWorldRotation {};
        glm::vec3 mWorldScale { 1.0f, 1.0f, 1.0f };

        glm::mat4 mWorldTransform {};
        glm::mat4 mInverseWorldTransform {};

        std::weak_ptr<TransformComponent> mParentTransform {};

        SignalId mParentTransformChangeListenerId {};
    };

    using Transform = TransformComponent;

}
