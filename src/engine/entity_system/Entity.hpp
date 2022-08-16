#pragma once

#include "Component.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/BedrockSignal.hpp"
#include "EntitySystemTypes.hpp"

#include "libs/nlohmann/json_fwd.hpp"

#include <memory>
#include <utility>
#include <vector>

// This class will need optimization in future

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

        void LateInit(bool triggerSignal = true);

        void Update(float deltaTimeInSec);

        void Shutdown(bool shouldNotifyParent = true);

        template<typename ComponentClass, typename ... ArgsT>
        std::shared_ptr<ComponentClass> AddComponent(ArgsT && ... args)
        {
            auto existingComponent = GetComponent<ComponentClass>();
            if (existingComponent != nullptr)
            {
                MFA_LOG_WARN("Component with name %s alreay exists", ComponentClass::Name);
                return existingComponent;
            }
            auto newComponent = std::make_shared<ComponentClass>(std::forward<ArgsT>(args)...);
            mComponents.emplace_back(newComponent);
            LinkComponent(newComponent.get());
            return newComponent;
        }

        bool AddComponent(std::shared_ptr<Component> const & component);

        bool RemoveComponent(std::string const & componentName);

        bool RemoveComponent(Component * component);

        template<typename ComponentClass>
        bool RemoveComponent()
        {
            for (int i = static_cast<int>(mComponents.size()) - 1; i >= 0; --i)
            {
                auto castResult = std::dynamic_pointer_cast<ComponentClass>(mComponents[i]);
                if (castResult != nullptr)
                {
                    UnLinkComponent(mComponents[i].get());
                    mComponents.erase(mComponents.begin() + i);
                    return true;
                } 
            }
            return false;
        }

        template<typename ComponentClass>
        [[nodiscard]]
        std::shared_ptr<ComponentClass> GetComponent()
        {
            for (auto & component : mComponents)
            {
                auto castResult = std::dynamic_pointer_cast<ComponentClass>(component);
                if (castResult != nullptr)
                {
                    return castResult;
                }   
            }
            return {};
        }

        // TODO: We need GetComponentsInChildren

        [[nodiscard]]
        std::shared_ptr<Component> GetComponent(std::string const & componentName) const;

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

        void NotifyANewChildAdded(Entity * entity);

        void NotifyAChildRemoved(Entity * entity);

        int FindDirectChild(Entity * entity);

        void LinkComponent(Component * component);

        void UnLinkComponent(Component * component);

        void UpdateEntity();


    public:

        Signal<Entity *> EditorSignal{};

    private:

        EntityId const mId;
        std::string mName{};
        Entity * mParent = nullptr;
        bool mSerializable = true;

        std::vector<std::shared_ptr<Component>> mComponents{};

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
