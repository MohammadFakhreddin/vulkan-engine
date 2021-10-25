#include "Entity.hpp"

#include "engine/ui_system/UISystem.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    Entity::Entity(
        char const * name,
        Entity * parent
    )
        : mName(name)
        , mParent(parent)
    {}

    //-------------------------------------------------------------------------------------------------

    Entity::~Entity() = default;

    //-------------------------------------------------------------------------------------------------

    void Entity::Init()
    {
        if (mIsInitialized)
        {
            return;
        }
        mIsInitialized = true;

        if (mParent != nullptr)
        {
            mParent->notifyANewChildAdded(this);
        }
        mInitSignal.Emit();
    }

    //-------------------------------------------------------------------------------------------------
    // TODO Support for priority between components
    void Entity::Update(float deltaTimeInSec, RT::CommandRecordState const & recordState) const
    {
        if (mIsActive == false)
        {
            return;
        }
        mUpdateSignal.Emit(deltaTimeInSec, recordState);
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
            mParent->notifyAChildRemoved(this);
        }
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
        mActivationStatusChangeSignal.Emit(mIsActive);
        // TODO We can register/unregister from entity system update event
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
                for (auto const & keyValues : mComponents)
                {
                    keyValues.second->OnUI();
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
        mIsParentActive = isActive;
    }

    //-------------------------------------------------------------------------------------------------

    void Entity::notifyANewChildAdded(Entity * entity)
    {
        MFA_ASSERT(entity != nullptr);
        MFA_ASSERT(findChild(entity) < 0);
        entity->mIsParentActive = IsActive();
        entity->mParentActivationStatusChangeListenerId = mActivationStatusChangeSignal.Register([entity](bool const isActive)->void
        {
            entity->OnParentActivationStatusChanged(isActive);
        });
        mChildEntities.emplace_back(entity);
    }

    //-------------------------------------------------------------------------------------------------

    void Entity::notifyAChildRemoved(Entity * entity)
    {
        mActivationStatusChangeSignal.UnRegister(entity->mParentActivationStatusChangeListenerId);
        auto const childIndex = findChild(entity);
        if (MFA_VERIFY(childIndex >= 0))
        {
            mChildEntities[childIndex] = mChildEntities.back();
            mChildEntities.pop_back();
        }
    }

    //-------------------------------------------------------------------------------------------------

    int Entity::findChild(Entity * entity)
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

    void Entity::linkComponent(Component * component)
    {
        // Linked entity
        component->mEntity = this;
        // Init event
        if ((component->RequiredEvents() & Component::EventTypes::InitEvent) > 0)
        {
            component->mInitEventId = mInitSignal.Register([component]()->void { component->Init(); });
        }
        // Update event
        if ((component->RequiredEvents() & Component::EventTypes::UpdateEvent) > 0)
        {
            component->mUpdateEventId = mUpdateSignal.Register([component](
                float const deltaTimeInSec,
                RT::CommandRecordState const & recordState)->void
            {
                component->Update(deltaTimeInSec, recordState);
            });
        }
        // Shutdown event
        if ((component->RequiredEvents() & Component::EventTypes::ShutdownEvent) > 0)
        {
            component->mShutdownEventId = mShutdownSignal.Register([component]()->void
            {
                component->Shutdown();
            });
        }
    }

    //-------------------------------------------------------------------------------------------------

}
