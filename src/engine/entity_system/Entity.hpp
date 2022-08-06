#pragma once

#include "Component.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/BedrockSignal.hpp"
#include "EntitySystemTypes.hpp"

#include "libs/nlohmann/json_fwd.hpp"

#include <memory>
#include <utility>
#include <vector>

// TODO This class will need optimization in future

namespace MFA::EntitySystem
{
    void InitEntity(Entity * entity, bool triggerSignals);
    void UpdateEntity(Entity * entity);
    static void destroyEntity(EntityId const entityId, bool const shouldNotifyParent);
}

namespace MFA
{

    class Entity
    {
    public:
        friend void EntitySystem::InitEntity(Entity * entity, bool triggerSignals);
        friend void EntitySystem::UpdateEntity(Entity * entity);
        friend void EntitySystem::destroyEntity(EntityId const entityId, bool const shouldNotifyParent);
        friend Component;

        explicit Entity(
            EntityId id,
            std::string name,
            Entity * parent,
            bool serializable
        );

        ~Entity();

        Entity(Entity const &) noexcept = delete;
        Entity(Entity &&) noexcept = delete;
        Entity & operator = (Entity const &) noexcept = delete;
        Entity & operator = (Entity && rhs) noexcept = delete;

        [[nodiscard]]
        bool NeedUpdateEvent() noexcept
        {
            return mUpdateSignal.IsEmpty() == false && IsActive();
        }

        void Init(bool triggerSignal = true);

        void LateInit(bool triggerInitSignal = true);

        void Update(float deltaTimeInSec);

        void Shutdown(bool shouldNotifyParent = true);

        template<typename ComponentClass, typename ... ArgsT>
        std::weak_ptr<ComponentClass> AddComponent(ArgsT && ... args)
        {
            if (mComponents[ComponentClass::Family] != nullptr)
            {
                MFA_LOG_WARN("Component with type %d alreay exists", ComponentClass::Family);
                return GetComponent<ComponentClass>();
            }
            auto sharedPtr = std::make_shared<ComponentClass>(std::forward<ArgsT>(args)...);
            mComponents[ComponentClass::Family] = sharedPtr;
            LinkComponent(sharedPtr.get());
            return std::weak_ptr<ComponentClass>(sharedPtr);
        }

        void AddComponent(std::shared_ptr<Component> const & component)
        {
            MFA_ASSERT(mComponents[component->getFamily()] == nullptr);
            mComponents[component->getFamily()] = component;
            LinkComponent(component.get());
        }

        template<typename ComponentClass>
        void RemoveComponent(ComponentClass * component)
        {
            MFA_ASSERT(component != nullptr);
            MFA_ASSERT(component->mEntity == this);

            UnLinkComponent(component);

            auto & findResult = mComponents[component->getFamily()];
            MFA_ASSERT(findResult != nullptr);
            findResult = nullptr;
        }

        template<typename ComponentClass>
        [[nodiscard]]
        std::weak_ptr<ComponentClass> GetComponent()
        {
            auto & component = mComponents[ComponentClass::Family];
            if (component != nullptr)
            {
                return std::static_pointer_cast<ComponentClass>(component);
            }
            return std::weak_ptr<ComponentClass>();
        }

        [[nodiscard]]
        std::weak_ptr<Component> GetComponent(int const familyType) const;

        [[nodiscard]]
        EntityId getId() const noexcept;

        [[nodiscard]]
        Entity * GetParent() const noexcept;

        [[nodiscard]]
        std::string const & GetName() const noexcept;

        void SetName(char const * name);

        [[nodiscard]]
        bool IsActive() const noexcept;

        void SetActive(bool isActive);

        [[nodiscard]]
        std::vector<Entity *> const & GetChildEntities() const;

        void OnUI();

        [[nodiscard]]
        bool HasParent() const;

        void OnParentActivationStatusChanged(bool isActive);

        Entity * Clone(char const * name, Entity * parent = nullptr) const;

        void Serialize(nlohmann::json & jsonObject);

        void Deserialize(nlohmann::json const & jsonObject, bool initialize);

        std::vector<Component *> GetComponents();

        [[nodiscard]]
        bool IsSerializable() const;

    private:

        void notifyANewChildAdded(Entity * entity);

        void notifyAChildRemoved(Entity * entity);

        int findChild(Entity * entity);

        void LinkComponent(Component * component);

        void UnLinkComponent(Component * component);

        void onActivationStatusChanged();

    
    public:

        Signal<Entity *> EditorSignal{};

    private:

        EntityId const mId;
        std::string mName{};
        Entity * mParent = nullptr;
        bool mSerializable = true;

        std::shared_ptr<Component> mComponents[static_cast<int>(Component::FamilyType::Count)]{};

        Signal<> mInitSignal{};
        Signal<> mLateInitSignal{};
        Signal<float> mUpdateSignal{};
        Signal<> mShutdownSignal{};
        Signal<bool> mActivationStatusChangeSignal{};


        bool mIsActive = true;
        bool mIsParentActive = true;    // It should be true by default because not everyone have parent

        SignalId mUpdateListenerId = InvalidSignalId;

        int mParentActivationStatusChangeListenerId = 0;

        std::vector<Entity *> mChildEntities{};

        bool mIsInitialized = false;
    
    };

}
