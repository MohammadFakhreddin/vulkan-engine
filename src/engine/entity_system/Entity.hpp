#pragma once

#include "Component.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/BedrockSignal.hpp"

#include <memory>
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

    void Update(float deltaTimeInSec, RT::CommandRecordState const & recordState) const;
    
    void Shutdown(bool shouldNotifyParent = true);

    template<typename ComponentClass, typename ... ArgsT>
    std::weak_ptr<ComponentClass> AddComponent(ArgsT && ... args) {
        MFA_ASSERT(GetComponent<ComponentClass>().expired() == true);

        auto const & classTypes = ComponentClass::GetClassType();
        if (MFA_VERIFY(classTypes.empty() == false)) {
            auto sharedPtr = std::make_shared<ComponentClass>(std::forward<ArgsT>(args)...);
            MFA_ASSERT(mComponents[static_cast<int>(classTypes[0])] == nullptr);
            mComponents[static_cast<int>(classTypes[0])] = sharedPtr;
            sharedPtr->mSelfPtr = sharedPtr;
            linkComponent(sharedPtr.get());
            return std::weak_ptr<ComponentClass>(sharedPtr);
        }

        return std::weak_ptr<ComponentClass>();
    }

    template<typename ComponentClass>
    [[nodiscard]]
    std::weak_ptr<ComponentClass> GetComponent() {

        auto const & classTypes = ComponentClass::GetClassType();
        MFA_ASSERT(classTypes.empty() == false);
        for (auto & classType : classTypes)
        {
            auto & component = mComponents[static_cast<int>(classType)];
            if (component != nullptr)
            {
                return std::static_pointer_cast<ComponentClass>(component);        
            }
        }
        return std::weak_ptr<ComponentClass>();
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

    void SetActive(bool isActive);

    [[nodiscard]]
    std::vector<Entity *> const & GetChildEntities() const;

    void OnUI();

    [[nodiscard]]
    bool HasParent() const;

    void OnParentActivationStatusChanged(bool isActive);

private:

    void notifyANewChildAdded(Entity * entity);

    void notifyAChildRemoved(Entity * entity);

    int findChild(Entity * entity);

    void linkComponent(Component * component);

    std::string mName {};
    Entity * mParent = nullptr;

    std::shared_ptr<Component> mComponents [static_cast<int>(Component::ClassType::Count)]{};

    Signal<> mInitSignal {};
    Signal<float, RT::CommandRecordState const &> mUpdateSignal {};
    Signal<> mShutdownSignal {};
    Signal<bool> mActivationStatusChangeSignal {};

    bool mIsActive = true;
    bool mIsParentActive = true;    // It should be true by default because not everyone have parent

    int mUpdateListenerId = 0;

    int mParentActivationStatusChangeListenerId = 0;

    std::vector<Entity *> mChildEntities {};

    bool mIsInitialized = false;
    
};

}
