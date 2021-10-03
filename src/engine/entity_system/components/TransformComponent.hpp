#pragma once

#include "engine/entity_system/Component.hpp"
#include "engine/BedrockSignal.hpp"

#include <ext/matrix_transform.hpp>

namespace MFA
{

    class TransformComponent final : public Component
    {
    public:

        [[nodiscard]]
        EventType RequiredEvents() const override
        {
            return EventTypes::InitEvent | EventTypes::ShutdownEvent;
        }

        static uint8_t GetClassType(ClassType outComponentTypes[3])
        {
            outComponentTypes[0] = ClassType::TransformComponent;
            return 1;
        }
    
        void Init() override;

        void Shutdown() override;

        void UpdateTransform(float position[3], float rotation[3], float scale[3]);

        void UpdatePosition(float position[3]);

        void UpdateRotation(float rotation[3]);

        void UpdateScale(float scale[3]);

        [[nodiscard]]
        glm::mat4 const & GetTransform() const noexcept;

        void GetPosition(float outPosition[3]) const;

        void GetRotation(float outRotation[3]) const;

        void GetScale(float outScale[3]) const;

        Signal<>::ListenerId RegisterChangeListener(std::function<void()> const & listener);

        bool UnRegisterChangeListener(Signal<>::ListenerId listenerId);

    private:

        void computeTransform();

        Signal<> mTransformChangeSignal {};

        float mPosition[3]{ 0.0f, 0.0f, 0.0f };
        float mRotation[3]{ 0.0f, 0.0f, 0.0f };          // In euler angle // TODO Use Quaternion instead! Soon! ToQuat is heavy
        float mScale[3]{ 1.0f, 1.0f, 1.0f };

        glm::mat4 mTransform = glm::identity<glm::mat4>();

        TransformComponent * mParentTransform = nullptr;

        Signal<>::ListenerId mParentTransformChangeListenerId {};
    };

}
