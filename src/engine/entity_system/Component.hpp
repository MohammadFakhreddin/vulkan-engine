#pragma once

#include "engine/render_system/RenderTypesFWD.hpp"
#include "engine/BedrockSignal.hpp"

#include <cstdint>

namespace MFA
{

#define MFA_COMPONENT_PROPS(ComponentType)                          \
public:                                                             \
std::weak_ptr<ComponentType> SelfPtr()                              \
{                                                                   \
    return std::static_pointer_cast<ComponentType>(mSelfPtr.lock());\
}                                                                   \

#define MFA_COMPONENT_CLASS_TYPE(classType)                         \
public:                                                             \
static constexpr int Type = static_cast<int>(classType);            \
                                                                    \
[[nodiscard]]                                                       \
int GetType() override                                              \
{                                                                   \
    return Type;                                                    \
}                                                                   \

#define MFA_COMPONENT_REQUIRED_EVENTS(eventTypes)                   \
public:                                                             \
[[nodiscard]]                                                       \
EventType RequiredEvents() const override                           \
{                                                                   \
    return eventTypes;                                              \
}                                                                   \

class Entity;

class Component
{
public:

    friend Entity;

    using EventType = uint8_t;

    struct EventTypes {
        static constexpr EventType EmptyEvent = 0b0;
        static constexpr EventType InitEvent = 0b1;
        static constexpr EventType UpdateEvent = 0b10;
        static constexpr EventType ShutdownEvent = 0b100;
    };

    virtual ~Component();

    Component (Component const &) noexcept = delete;
    Component (Component &&) noexcept = delete;
    Component & operator = (Component const &) noexcept = delete;
    Component & operator = (Component && rhs) noexcept = delete;

    [[nodiscard]]
    virtual EventType RequiredEvents() const
    {
        return EventTypes::EmptyEvent;
    }

    enum class ClassType : uint8_t
    {
        Invalid,

        TransformComponent,

        MeshRendererComponent,

        BoundingVolumeRendererComponent,

        BoundingVolumeComponent,
        //SphereBoundingVolumeComponent,
        //AxisAlignedBoundingBoxes,

        ColorComponent,

        CameraComponent,
        //ObserverCameraComponent,
        //ThirdPersonCamera,

        PointLightComponent,
        DirectionalLightComponent,

        Count
    };

    virtual int GetType() = 0;

    virtual void Init();

    virtual void Update(float deltaTimeInSec, RT::CommandRecordState const & recordState);
    
    virtual void Shutdown();

    void SetActive(bool isActive);

    [[nodiscard]]
    bool IsActive() const;
    
    [[nodiscard]]
    Entity * GetEntity() const
    {
        return mEntity;
    }

    virtual void OnUI();

    Signal<Component *, Entity *> EditorSignal {};

protected:

    explicit Component();

    std::weak_ptr<Component> mSelfPtr {};

private:

    Entity * mEntity = nullptr;

    bool mIsActive = true;

    int mInitEventId = 0;

    int mUpdateEventId = 0;

    int mShutdownEventId = 0;

};

}
