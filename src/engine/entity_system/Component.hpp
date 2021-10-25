#pragma once

#include "engine/render_system/RenderTypesFWD.hpp"

#include <cstdint>

namespace MFA
{

#define MFA_COMPONENT_PROPS(ComponentType)                          \
public:                                                             \
std::weak_ptr<ComponentType> SelfPtr()                              \
{                                                                   \
    return std::static_pointer_cast<ComponentType>(mSelfPtr.lock());\
}                                                                   \

#define MFA_COMPONENT_CLASS_TYPE_1(type1)                           \
public:                                                             \
static uint8_t GetClassType(ClassType outComponentTypes[3])         \
{                                                                   \
    outComponentTypes[0] = type1;                                   \
    return 1;                                                       \
}                                                                   \

#define MFA_COMPONENT_CLASS_TYPE_2(type1, type2)                    \
public:                                                             \
static uint8_t GetClassType(ClassType outComponentTypes[3])         \
{                                                                   \
    outComponentTypes[0] = type1;                                   \
    outComponentTypes[1] = type2;                                   \
    return 2;                                                       \
}                                                                   \

#define MFA_COMPONENT_CLASS_TYPE_3(type1, type2, type3)             \
public:                                                             \
static uint8_t GetClassType(ClassType outComponentTypes[3])         \
{                                                                   \
    outComponentTypes[0] = type1;                                   \
    outComponentTypes[1] = type2;                                   \
    outComponentTypes[2] = type3;                                   \
    return 3;                                                       \
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

    enum class ClassType
    {
        Invalid,

        TransformComponent,

        MeshRendererComponent,
        BoundingVolumeRendererComponent,

        BoundingVolumeComponent,
        SphereBoundingVolumeComponent,
        AxisAlignedBoundingBoxes,

        ColorComponent,

        CameraComponent,
        ObserverCameraComponent,
        ThirdPersonCamera,

        PointLightComponent,
        DirectionalLightComponent
    };

    static uint8_t GetClassType(ClassType outComponentTypes[3])
    {
        return 0;
    }
    
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
