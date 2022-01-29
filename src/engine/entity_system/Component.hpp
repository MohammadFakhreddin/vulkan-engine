#pragma once

#include "engine/render_system/RenderTypesFWD.hpp"
#include "engine/BedrockSignal.hpp"

#include "libs/nlohmann/json_fwd.hpp"

#include <cstdint>

namespace MFA
{

#define MFA_COMPONENT_PROPS(componentName, family, eventTypes)          \
public:                                                                 \
                                                                        \
static constexpr char const * Name = #componentName;                    \
static constexpr int Family = static_cast<int>(family);                 \
                                                                        \
std::weak_ptr<componentName> selfPtr()                                  \
{                                                                       \
    return std::static_pointer_cast<componentName>(shared_from_this()); \
}                                                                       \
                                                                        \
[[nodiscard]]                                                           \
char const * getName() override                                         \
{                                                                       \
    return Name;                                                        \
}                                                                       \
                                                                        \
[[nodiscard]]                                                           \
int getFamily() override                                                \
{                                                                       \
    return Family;                                                      \
}                                                                       \
                                                                        \
[[nodiscard]]                                                           \
EventType requiredEvents() const override                               \
{                                                                       \
    return eventTypes;                                                  \
}                                                                       \
                                                                        \
componentName (componentName const &) noexcept = delete;                \
componentName (componentName &&) noexcept = delete;                     \
componentName & operator = (componentName const &) noexcept = delete;   \
componentName & operator = (componentName && rhs) noexcept = delete;    \

class Entity;

class Component : public std::enable_shared_from_this<Component>
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
    virtual EventType requiredEvents() const
    {
        return EventTypes::EmptyEvent;
    }

    enum class FamilyType : uint8_t
    {
        Invalid,

        Transform,

        MeshRenderer,
        BoundingVolumeRenderer,
        
        BoundingVolume,
        //SphereBoundingVolumeComponent,
        //AxisAlignedBoundingBoxes,

        Color,

        Camera,
        //ObserverCameraComponent,
        //ThirdPersonCamera,

        PointLight,
        DirectionalLight,

        Count
    };

    virtual char const * getName() = 0;

    virtual int getFamily() = 0;

    virtual void init();

    virtual void Update(float deltaTimeInSec);
    
    virtual void shutdown();

    void SetActive(bool isActive);

    [[nodiscard]]
    bool IsActive() const;
    
    [[nodiscard]]
    Entity * GetEntity() const
    {
        return mEntity;
    }

    virtual void onUI();

    virtual void clone(Entity * entity) const = 0;

    virtual void serialize(nlohmann::json & jsonObject) const = 0;

    virtual void deserialize(nlohmann::json const & jsonObject) = 0;

    Signal<Component *, Entity *> EditorSignal {};

protected:

    explicit Component();
    
private:

    Entity * mEntity = nullptr;

    bool mIsActive = true;

    int mInitEventId = 0;

    int mUpdateEventId = 0;

    int mShutdownEventId = 0;

};

}
