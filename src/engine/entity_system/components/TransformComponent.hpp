#pragma once

#include "engine/entity_system/Component.hpp"
#include "engine/BedrockSignal.hpp"

#include <mat4x4.hpp>

namespace MFA
{

    class TransformComponent final : public Component
    {
    public:

        MFA_COMPONENT_PROPS(TransformComponent)
        MFA_COMPONENT_CLASS_TYPE(ClassType::TransformComponent)
        MFA_COMPONENT_REQUIRED_EVENTS(EventTypes::InitEvent | EventTypes::ShutdownEvent)

        explicit TransformComponent();

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

        void GetPosition(float outPosition[3]) const;

        void GetRotation(float outRotation[3]) const;

        void GetScale(float outScale[3]) const;

        [[nodiscard]]
        glm::vec3 const & GetPosition() const;

        [[nodiscard]]
        glm::vec3 const & GetRotation() const;

        [[nodiscard]]
        glm::vec3 const & GetScale() const;

        Signal<>::ListenerId RegisterChangeListener(std::function<void()> const & listener);

        bool UnRegisterChangeListener(Signal<>::ListenerId listenerId);

        void OnUI() override;

    private:

        void computeTransform();

        Signal<> mTransformChangeSignal {};

        glm::vec3 mPosition { 0.0f, 0.0f, 0.0f };
        glm::vec3 mRotation { 0.0f, 0.0f, 0.0f };          // In euler angle // TODO Use Quaternion instead! Soon! ToQuat is heavy
        glm::vec3 mScale { 1.0f, 1.0f, 1.0f };

        glm::mat4 mTransform;

        std::weak_ptr<TransformComponent> mParentTransform {};

        Signal<>::ListenerId mParentTransformChangeListenerId {};
    };

}
