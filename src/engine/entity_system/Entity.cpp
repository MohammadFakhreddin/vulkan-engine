#include "Entity.hpp"

#include "EntitySystem.hpp"
#include "engine/ui_system/UI_System.hpp"

#include "libs/nlohmann/json.hpp"

namespace MFA
{

    using namespace EntitySystem;
    
    //-------------------------------------------------------------------------------------------------
    
    Entity::Entity(
        EntityId const id,
        std::string name,
        Entity * parent,
        bool const serializable
    )
        : mId(id)
        , mName(std::move(name))
        , mParent(parent)
        , mSerializable(serializable)
    {}

    //-------------------------------------------------------------------------------------------------

    Entity::~Entity() = default;

    //-------------------------------------------------------------------------------------------------

    void Entity::Init(bool const triggerSignal)
    {
        if (mIsInitialized)
        {
            return;
        }
        mIsInitialized = true;

        if (mParent != nullptr)
        {
            mParent->NotifyANewChildAdded(this);
        }

        if (triggerSignal == true)
        {
            mInitSignal.Emit();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void Entity::LateInit(bool const triggerSignal)
    {
        if (triggerSignal)
        {
            mLateInitSignal.Emit();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void Entity::Update(float const deltaTimeInSec)
    {
        if (mIsActive == false)
        {
            return;
        }
        mUpdateSignal.EmitMultiThread(deltaTimeInSec);
    }

    //-------------------------------------------------------------------------------------------------

    void Entity::Shutdown(bool const shouldNotifyParent)
    {
        if (mIsInitialized == false)
        {
            return;
        }
        mIsInitialized = false;

        mShutdownSignal.Emit();

        if (shouldNotifyParent && mParent != nullptr)
        {
            mParent->NotifyAChildRemoved(this);
        }
    }

    //-------------------------------------------------------------------------------------------------

    bool Entity::AddComponent(std::shared_ptr<Component> const & component)
    {
        MFA_ASSERT(component != nullptr);

        if (GetComponent(component->GetName()) != nullptr)
        {
            MFA_ASSERT(false);
            return false;
        }

        mComponents.emplace_back(component);
        LinkComponent(component.get());

        return true;
    }

    //-------------------------------------------------------------------------------------------------

    bool Entity::RemoveComponent(std::string const & componentName)
    {
        for (int i = static_cast<int>(mComponents.size()) - 1; i >= 0; --i)
        {
            if (mComponents[i]->HaveSameNameOrParent(componentName))
            {
                UnLinkComponent(mComponents[i].get());
                mComponents.erase(mComponents.begin() + i);
                return true;
            }   
        }
        return false;
    }

    //-------------------------------------------------------------------------------------------------

    bool Entity::RemoveComponent(Component * component)
    {
        MFA_ASSERT(component != nullptr);
        MFA_ASSERT(component->mEntity == this);

        UnLinkComponent(component);

        bool removed = false;
        for (int i = static_cast<int>(mComponents.size()) - 1; i >= 0; --i)
        {
            if (mComponents[i].get() == component)
            {
                MFA_ASSERT(mComponents[i]->GetName() == component->GetName());
                mComponents.erase(mComponents.begin() + i);
                removed = true;
                break;
            }
        }

        return removed;
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<Component> Entity::GetComponent(std::string const & componentName) const
    {
        for (auto & component : mComponents)
        {
            if (component->HaveSameNameOrParent(componentName))
            {
                return component;
            }   
        }
        return {};
    }

    //-------------------------------------------------------------------------------------------------

    EntityId Entity::getId() const noexcept
    {
        return mId;
    }

    //-------------------------------------------------------------------------------------------------

    Entity * Entity::GetParent() const noexcept
    {
        return mParent;
    }
    
    //-------------------------------------------------------------------------------------------------

    std::string const & Entity::GetName() const noexcept
    {
        return mName;
    }

    //-------------------------------------------------------------------------------------------------

    void Entity::SetName(char const * name)
    {
        MFA_ASSERT(name != nullptr && strlen(name) > 0);
        mName = name;
    }

    //-------------------------------------------------------------------------------------------------

    bool Entity::IsActive() const noexcept
    {
        return mIsActive && mIsParentActive;
    }

    //-------------------------------------------------------------------------------------------------

    void Entity::SetActive(bool const isActive)
    {
        if (isActive == mIsActive)
        {
            return;
        }
        mIsActive = isActive;
        mActivationStatusChangeSignal.EmitMultiThread(mIsActive);
        onActivationStatusChanged();
    }

    //-------------------------------------------------------------------------------------------------

    std::vector<Entity *> const & Entity::GetChildEntities() const
    {
        return mChildEntities;
    }

    //-------------------------------------------------------------------------------------------------

    void Entity::OnUI()
    {
        if (UI::TreeNode(mName.c_str()))
        {
            bool isActive = mIsActive;
            UI::Checkbox("IsActive", &isActive);
            SetActive(isActive);
            if (UI::TreeNode("Components"))
            {
                for (auto const & component : mComponents)
                {
                    if (component != nullptr)
                    {
                        component->OnUI();
                    }
                }
                UI::TreePop();
            }
            if (UI::TreeNode("Children"))
            {
                for (auto * childEntity : mChildEntities)
                {
                    childEntity->OnUI();
                }
                UI::TreePop();
            }
            EditorSignal.Emit(this);
            UI::TreePop();
        }
    }

    //-------------------------------------------------------------------------------------------------

    bool Entity::HasParent() const
    {
        return mParent != nullptr;
    }

    //-------------------------------------------------------------------------------------------------

    void Entity::OnParentActivationStatusChanged(bool const isActive)
    {
        if (mIsParentActive == isActive)
        {
            return;
        }
        mIsParentActive = isActive;
        if (isActive == true)   // If we are already in-active we remain inactive
        {
            mActivationStatusChangeSignal.Emit(IsActive());
            onActivationStatusChanged();
        }
    }

    //-------------------------------------------------------------------------------------------------

    Entity * Entity::Clone(char const * name, Entity * parent) const
    {
        MFA_ASSERT(name != nullptr);
        MFA_ASSERT(strlen(name) > 0);

        auto * entity = EntitySystem::CreateEntity(name, parent);
        entity->mIsActive = mIsActive;

        for (auto & component : mComponents)
        {
            if (component != nullptr)
            {
                component->Clone(entity);
            }
        }
        for (auto & child : mChildEntities)
        {
            child->Clone(child->GetName().c_str(), entity);
        }

        EntitySystem::InitEntity(entity);

        return entity;
    }

    //-------------------------------------------------------------------------------------------------

    void Entity::Serialize(nlohmann::json & jsonObject)
    {
        MFA_ASSERT(mSerializable == true);
        jsonObject["isActive"] = mIsActive;
        jsonObject["name"] = mName.c_str();
        for (auto const & component : mComponents)
        {
            if (component != nullptr)
            {
                nlohmann::json componentJson{};
                componentJson["name"] = component->GetName();

                nlohmann::json componentData{};
                component->Serialize(componentData);
                componentJson["data"] = componentData;

                jsonObject["components"].emplace_back(componentJson);

            }
        }

        for (auto const & child : mChildEntities)
        {
            if (child->IsSerializable())
            {
                nlohmann::json childEntityJson{};
                child->Serialize(childEntityJson);
                jsonObject["children"].emplace_back(childEntityJson);
            }
        }
    }

    //-------------------------------------------------------------------------------------------------

    void Entity::Deserialize(nlohmann::json const & jsonObject, bool const initialize)
    {
        mName = jsonObject.value("name", "undefined");
        mIsActive = jsonObject.value("isActive", true);
        auto const & rawComponents = jsonObject.value<std::vector<nlohmann::json>>("components", {});
        for (auto const & rawComponent : rawComponents)
        {
            std::string const name = rawComponent.value("name", "undefined");
            
            auto rawComponentData = rawComponent["data"];

            std::shared_ptr<Component> component = Component::CreateComponent(name);

            if (MFA_VERIFY(component != nullptr))
            {
                AddComponent(component);
                component->Deserialize(rawComponentData);
            }
        }

        auto const & rawChildren = jsonObject.value<std::vector<nlohmann::json>>("children", {});

        for (auto const & rawChild : rawChildren)
        {
            auto * entity = EntitySystem::CreateEntity("prefab-child", this);
            entity->Deserialize(rawChild, initialize);
        }

        EntitySystem::InitEntity(this, initialize);
    }

    //-------------------------------------------------------------------------------------------------

    std::vector<Component *> Entity::GetComponents()
    {
        std::vector<Component *> result {};
        for (auto & component : mComponents)
        {
            if (component != nullptr)
            {
                result.emplace_back(component.get());
            }
        }
        return result;
    }

    //-------------------------------------------------------------------------------------------------

    bool Entity::IsSerializable() const
    {
        return mSerializable;
    }

    //-------------------------------------------------------------------------------------------------

    void Entity::NotifyANewChildAdded(Entity * entity)
    {
        MFA_ASSERT(entity != nullptr);
        MFA_ASSERT(FindDirectChild(entity) < 0);
        entity->mIsParentActive = IsActive();
        entity->mParentActivationStatusChangeListenerId = mActivationStatusChangeSignal.Register([entity](bool const isActive)->void
        {
            entity->OnParentActivationStatusChanged(isActive);
        });
        mChildEntities.emplace_back(entity);
    }

    //-------------------------------------------------------------------------------------------------

    void Entity::NotifyAChildRemoved(Entity * entity)
    {
        mActivationStatusChangeSignal.UnRegister(entity->mParentActivationStatusChangeListenerId);
        auto const childIndex = FindDirectChild(entity);
        if (MFA_VERIFY(childIndex >= 0))
        {
            mChildEntities[childIndex] = mChildEntities.back();
            mChildEntities.pop_back();
        }
    }

    //-------------------------------------------------------------------------------------------------

    int Entity::FindDirectChild(Entity * entity)
    {
        MFA_ASSERT(entity != nullptr);
        for (int i = 0; i < static_cast<int>(mChildEntities.size()); ++i)
        {
            if (mChildEntities[i] == entity)
            {
                return i;
            }
        }
        return -1;
    }
    
    //-------------------------------------------------------------------------------------------------

    void Entity::LinkComponent(Component * component)
    {
        #define LINK_TO_EVENT(eventId, eventType, signal, callback)             \
        if ((requiredEvents & Component::EventTypes::eventType) > 0)            \
        {                                                                       \
            MFA_ASSERT(component->eventId == InvalidSignalId);                  \
            component->eventId = (signal).Register(callback);                   \
        }                                                                       \

        MFA_ASSERT(component != nullptr);

        auto const requiredEvents = component->requiredEvents();

        // Linked entity
        component->mEntity = this;
        // Init event
        LINK_TO_EVENT(
            mInitEventId,
            InitEvent,
            mInitSignal,
            [component]()->void{
                component->Init();
            }
        );
        // Late Init event
        LINK_TO_EVENT(
            mLateInitEventId,
            LateInitEvent,
            mLateInitSignal,
            [component]()->void{
                component->LateInit();
            }
        )
        // Update event
        LINK_TO_EVENT(
            mUpdateEventId,
            UpdateEvent,
            mUpdateSignal,
            [component](float const deltaTimeInSec)->void
            {
                component->Update(deltaTimeInSec);
            }
        )
        // Shutdown event
        LINK_TO_EVENT(
            mShutdownEventId,
            ShutdownEvent,
            mShutdownSignal,
            [component]()->void
            {
                component->Shutdown();
            }
        )
        // Activation change event
        LINK_TO_EVENT(
            mActivationChangeEventId,
            ActivationChangeEvent,
            mActivationStatusChangeSignal,
            [component](bool const isActive)->void
            {
                component->OnActivationStatusChanged(isActive);
            }
        )
    }

    //-------------------------------------------------------------------------------------------------

    void Entity::UnLinkComponent(Component * component)
    {
        MFA_ASSERT(component != nullptr);
        
        auto const requiredEvents = component->requiredEvents();

        #define UNLINK_FROM_EVENT(eventType, eventId, signal)               \
        if ((requiredEvents & Component::EventTypes::eventType) > 0)        \
        {                                                                   \
            (signal).UnRegister(component->eventId);                        \
        }                                                                   \

        // Init event
        UNLINK_FROM_EVENT(InitEvent, mInitEventId, mInitSignal)

        // Late init event
        UNLINK_FROM_EVENT(LateInitEvent, mLateInitEventId, mLateInitSignal)

        // Update event
        UNLINK_FROM_EVENT(UpdateEvent, mUpdateEventId, mUpdateSignal)

        // Shutdown event
        UNLINK_FROM_EVENT(ShutdownEvent, mShutdownEventId, mShutdownSignal)
        
        // Activation change event
        UNLINK_FROM_EVENT(ActivationChangeEvent, mActivationChangeEventId, mActivationStatusChangeSignal)
    }

    //-------------------------------------------------------------------------------------------------

    void Entity::onActivationStatusChanged()
    {
        if (mIsInitialized == false)
        {
            return;
        }
        EntitySystem::UpdateEntity(this);
    }

    //-------------------------------------------------------------------------------------------------

}
