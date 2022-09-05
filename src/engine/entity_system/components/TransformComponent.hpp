#pragma once

#include "engine/entity_system/Component.hpp"
#include "engine/BedrockSignal.hpp"
#include "engine/BedrockRotation.hpp"

#include <glm/gtx/quaternion.hpp>

namespace MFA
{

    class TransformComponent final : public Component
    {
    public:

        struct ChangeParams
        {
            bool worldPositionChanged;
            bool worldRotationChanged;
            bool worldScaleChanged;
        };

        MFA_COMPONENT_PROPS(
            TransformComponent,
            EventTypes::InitEvent | EventTypes::ShutdownEvent,
            Component
        )

        explicit TransformComponent(
            glm::vec3 const & localPosition_,
            glm::vec3 const & localEulerAngles_,          // Degrees
            glm::vec3 const & localScale_
        );
        
        explicit TransformComponent(
            glm::vec3 const & localPosition_,
            glm::quat const & localQuaternion_,
            glm::vec3 const & localScale_
        );

        explicit TransformComponent(
            glm::vec3 const & localPosition_,
            Rotation const & localRotation_,
            glm::vec3 const & scale_
        );

        void Init() override;

        void Shutdown() override;

        void SetLocalTransform(float position[3], float eulerAngles[3], float scale[3]);

        void SetLocalTransform(glm::vec3 const & position, glm::vec3 const & eulerAngles, glm::vec3 const & scale);

        void SetLocalTransform(glm::vec3 const & position, Rotation const & rotation, glm::vec3 const & scale);

        void SetLocalPosition(glm::vec3 const & position);

        void SetLocalPosition(float position[3]);

        void SetLocalRotation(glm::vec3 const & eulerAngle);

        void SetLocalRotation(float rotation[3]);

        void SetLocalRotation(glm::quat const & quaternion);

        void SetLocalScale(float scale[3]);
        
        void SetLocalScale(glm::vec3 const & scale);

        void SetWorldTransform(
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
        Rotation const & GetLocalRotation() const;
        
        [[nodiscard]]
        Rotation const & GetWorldRotation() const;

        [[nodiscard]]
        glm::vec3 const & GetLocalScale() const;

        [[nodiscard]]
        glm::vec3 const & GetWorldScale() const;

        SignalId RegisterChangeListener(std::function<void(ChangeParams const &)> const & listener);

        bool UnRegisterChangeListener(SignalId listenerId);

        void OnUI() override;

        void Clone(Entity * entity) const override;

        void Serialize(nlohmann::json & jsonObject) const override;

        void Deserialize(nlohmann::json const & jsonObject) override;

        [[nodiscard]]
        glm::vec3 Forward() const;

        [[nodiscard]]
        glm::vec3 Right() const;

        [[nodiscard]]
        glm::vec3 Up() const;

    private:

        void ComputeTransform();

        Signal<ChangeParams> mTransformChangeSignal {};

        glm::vec3 mLocalPosition { 0.0f, 0.0f, 0.0f };

        Rotation mLocalRotation{};

        glm::vec3 mLocalScale { 1.0f, 1.0f, 1.0f };

        glm::vec4 mWorldPosition {};

        Rotation mWorldRotation{};
        
        glm::vec3 mWorldScale { 1.0f, 1.0f, 1.0f };

        glm::mat4 mWorldTransform {};
        glm::mat4 mInverseWorldTransform {};

        std::weak_ptr<TransformComponent> mParentTransform {};

        SignalId mParentTransformChangeListenerId {};
    };

    using Transform = TransformComponent;

}
