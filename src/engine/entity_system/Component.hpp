#pragma once

#include "engine/render_system/RenderTypesFWD.hpp"
#include "engine/BedrockSignal.hpp"

#include "libs/nlohmann/json_fwd.hpp"

#include <cstdint>

namespace MFA
{

#define MFA_COMPONENT_PROPS(componentName, familyType, eventTypes)      \
public:                                                                 \
                                                                        \
static constexpr char const * Name = #componentName;                    \
static constexpr int FamilyType = static_cast<int>(familyType);         \
                                                                        \
std::weak_ptr<componentName> SelfPtr() const                            \
{                                                                       \
    return std::static_pointer_cast<componentName>(mSelfPtr.lock());    \
}                                                                       \
                                                                        \
[[nodiscard]]                                                           \
char const * GetName() override                                         \
{                                                                       \
    return Name;                                                        \
}                                                                       \
                                                                        \
[[nodiscard]]                                                           \
int GetFamilyType() override                                            \
{                                                                       \
    return FamilyType;                                                  \
}                                                                       \
                                                                        \
[[nodiscard]]                                                           \
EventType RequiredEvents() const override                               \
{                                                                       \
    return eventTypes;                                                  \
}                                                                       \
                                                                        \
componentName (componentName const &) noexcept = delete;                \
componentName (componentName &&) noexcept = delete;                     \
componentName & operator = (componentName const &) noexcept = delete;   \
componentName & operator = (componentName && rhs) noexcept = delete;    \

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

    enum class FamilyType : uint8_t
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

    virtual char const * GetName() = 0;

    virtual int GetFamilyType() = 0;

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

    virtual void Clone(Entity * entity) const = 0;

    virtual void Serialize(nlohmann::json & jsonObject) const = 0;

    virtual void Deserialize(nlohmann::json const & jsonObject) = 0;

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
