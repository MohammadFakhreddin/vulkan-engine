#pragma once

#include "engine/render_system/RenderTypesFWD.hpp"

#include <cstdint>

namespace MFA
{

class Entity;

class Component
{
public:

    using EventType = uint8_t;

    struct EventTypes {
        static constexpr EventType EmptyEvent = 0b0;
        static constexpr EventType InitEvent = 0b1;
        static constexpr EventType UpdateEvent = 0b10;
        static constexpr EventType ShutdownEvent = 0b100;
        static constexpr EventType ActivationChangeEvent = 0b1000;
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
        ThirdPersonCamera

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

private:

    friend Entity;
    Entity * mEntity = nullptr;

    bool mIsActive = true;

    friend Entity;
    int mInitEventId = 0;

    friend Entity;
    int mUpdateEventId = 0;

    friend Entity;
    int mShutdownEventId = 0;
};

}
