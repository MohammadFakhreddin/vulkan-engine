#pragma once

#include <functional>

#include "Component.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/BedrockSignal.hpp"

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

// TODO This class will need optimization in future

namespace MFA::EntitySystem
{
    void InitEntity(Entity * entity);
    bool DestroyEntity(Entity * entity, bool shouldNotifyParent);
}

namespace MFA {

class Entity {
public:
    friend void EntitySystem::InitEntity(Entity * entity);
    friend bool EntitySystem::DestroyEntity(Entity * entity, bool shouldNotifyParent);
    friend Component;

    explicit Entity(char const * name, Entity * parent = nullptr);

    ~Entity();

    Entity (Entity const &) noexcept = delete;
    Entity (Entity &&) noexcept = delete;
    Entity & operator = (Entity const &) noexcept = delete;
    Entity & operator = (Entity && rhs) noexcept = delete;

    [[nodiscard]]
    bool NeedUpdateEvent() const noexcept
    {
        return mUpdateSignal.IsEmpty() == false;
    }

    void Init();

    void Update(float deltaTimeInSec) const;
    
    void Shutdown(bool shouldNotifyParent = true);

    template<typename ComponentClass, typename ... ArgsT>
    ComponentClass * AddComponent(ArgsT && ... args) {
        MFA_ASSERT(GetComponent<ComponentClass>() == nullptr);

        Component::ClassType classTypes[3] {};
        uint8_t const typesCount = ComponentClass::GetClassType(classTypes);
        if (MFA_VERIFY(typesCount > 0)) {
            mComponents[classTypes[0]] = std::make_unique<ComponentClass>(std::forward<ArgsT>(args)...);

            auto * component = mComponents[classTypes[0]].get();
            linkComponent(component);
            return dynamic_cast<ComponentClass *>(component);
        }

        return nullptr;
    }

    template<typename ComponentClass>
    [[nodiscard]]
    ComponentClass * GetComponent() {

        Component::ClassType classTypes[3] {};
        uint8_t const typesCount = ComponentClass::GetClassType(classTypes);

        for (uint8_t i = 0; i < typesCount; ++i)
        {
            auto findResult = mComponents.find(classTypes[i]);
            if (findResult != mComponents.end())
            {
                return dynamic_cast<ComponentClass *>(findResult->second.get());        
            }
        }
        return nullptr;
    }

    [[nodiscard]]
    Entity * GetParent() const noexcept {
        return mParent;
    }

    [[nodiscard]]
    std::string const & GetName() const noexcept;

    void SetName(char const * name);

    [[nodiscard]]
    bool IsActive() const noexcept;

    void SetActive(bool active);

    [[nodiscard]]
    std::vector<Entity *> const & GetChildEntities() const;

private:

    void notifyANewChildAdded(Entity * entity);

    void notifyAChildRemoved(Entity * entity);

    int findChild(Entity * entity);

    void linkComponent(Component * component);

    std::string mName {};
    Entity * mParent = nullptr;

    std::unordered_map<Component::ClassType, std::unique_ptr<Component>> mComponents {};

    Signal<> mInitSignal {};
    Signal<float> mUpdateSignal {};
    Signal<> mShutdownSignal {};

    bool mIsActive = true;

    int mUpdateListenerId = 0;

    std::vector<Entity *> mChildEntities {};

    bool mIsInitialized = false;

};

}
