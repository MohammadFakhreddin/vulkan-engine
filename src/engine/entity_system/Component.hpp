#pragma once

#include "engine/BedrockSignal.hpp"

#include "libs/nlohmann/json_fwd.hpp"

#include <cstdint>

namespace MFA
{
    class Component;

    #define MFA_ABSTRACT_COMPONENT_PROPS(componentName, family, eventTypes, parent) \
    public:                                                                         \
                                                                                    \
    static constexpr char const * Name = #componentName;                            \
    static constexpr int Family = static_cast<int>(family);                         \
    static constexpr EventType RequiredEvents = eventTypes;                         \
                                                                                    \
    using parent::parent;                                                           \
                                                                                    \
    std::weak_ptr<componentName> selfPtr()                                          \
    {                                                                               \
        return std::static_pointer_cast<componentName>(shared_from_this());         \
    }                                                                               \
                                                                                    \
    [[nodiscard]]                                                                   \
    char const * getName() override                                                 \
    {                                                                               \
        return Name;                                                                \
    }                                                                               \
                                                                                    \
    [[nodiscard]]                                                                   \
    int getFamily() override                                                        \
    {                                                                               \
        return Family;                                                              \
    }                                                                               \
                                                                                    \
    [[nodiscard]]                                                                   \
    EventType requiredEvents() const override                                       \
    {                                                                               \
        return eventTypes | parent::requiredEvents();                               \
    }                                                                               \
                                                                                    \
    componentName (componentName const &) noexcept = delete;                        \
    componentName (componentName &&) noexcept = delete;                             \
    componentName & operator = (componentName const &) noexcept = delete;           \
    componentName & operator = (componentName && rhs) noexcept = delete;            \
                                                                                    \
    using Parent = parent;                                                          \


    #define MFA_COMPONENT_PROPS(componentName, family, eventTypes, parent)          \
    MFA_ABSTRACT_COMPONENT_PROPS(componentName, family, eventTypes, parent)         \
    private:                                                                        \
                                                                                    \
        static inline ComponentRegister<componentName> mComponentRegister {};       \
                                                                                    \
    public:                                                                         \

    class Entity;

    class Component : public std::enable_shared_from_this<Component>
    {
    public:

        friend Entity;

        using EventType = uint8_t;

        struct EventTypes
        {
            static constexpr EventType EmptyEvent = 0b0;
            static constexpr EventType InitEvent = 0b1;
            static constexpr EventType LateInitEvent = 0b10;
            static constexpr EventType UpdateEvent = 0b100;
            static constexpr EventType ShutdownEvent = 0b1000;
            static constexpr EventType ActivationChangeEvent = 0b10000;
        };

        explicit Component();

        virtual ~Component();

        Component(Component const &) noexcept = delete;
        Component(Component &&) noexcept = delete;
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

            OcclusionCulling,

            Collider,

            Rigidbody,

            Count
        };

        virtual char const * getName() = 0;

        virtual int getFamily() = 0;

        virtual void Init();

        virtual void LateInit();

        virtual void Update(float deltaTimeInSec);

        virtual void Shutdown();

        virtual void OnActivationStatusChanged(bool isActive);

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

        Signal<Component *, Entity *> EditorSignal{};

    private:

        Entity * mEntity = nullptr;

        bool mIsActive = true;

        SignalId mInitEventId = InvalidSignalId;

        SignalId mLateInitEventId = InvalidSignalId;

        SignalId mUpdateEventId = InvalidSignalId;

        SignalId mShutdownEventId = InvalidSignalId;

        SignalId mActivationChangeEventId = InvalidSignalId;

        static void RegisterComponent(
            std::string const & name,
            std::function<std::shared_ptr<Component>()> const & recipe
        );

        static std::shared_ptr<Component> CreateComponent(std::string const & name);

    protected:

        template<typename T>
        class ComponentRegister
        {
        public:
            explicit ComponentRegister()
            {
                RegisterComponent(T::Name, [](){
                    return std::make_shared<T>();
                });
            }
        };
    };
    


}
