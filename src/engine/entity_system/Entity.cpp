#include "Entity.hpp"

namespace MFA {

//-------------------------------------------------------------------------------------------------

Entity::Entity(
    char const * name,
    Entity * parent
)
    : mName(name)
    , mParent(parent)
{
}

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
void Entity::Update(float deltaTimeInSec) const
{
    if (mIsActive == false)
    {
        return;
    }
    mUpdateSignal.Emit(deltaTimeInSec);
}

//-------------------------------------------------------------------------------------------------

void Entity::Shutdown(bool shouldNotifyParent)
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
    //for (auto * childEntity : mChildEntities)
    //{
    //    if (childEntity != nullptr)
    //    {
    //        childEntity->Shutdown();
    //    }
    //}
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
    if (mIsActive == false)
    {
        return false;
    }
    Entity * parent = mParent;
    while (parent != nullptr)
    {
        if (parent->IsActive() == false)
        {
            return false;
        }
        parent = mParent->GetParent();
    }
    return true;
}

//-------------------------------------------------------------------------------------------------

void Entity::SetActive(bool const active)
{
    mIsActive = active;
    // TODO We can register/unregister from entity system update event
}

//-------------------------------------------------------------------------------------------------

std::vector<Entity *> const & Entity::GetChildEntities() const
{
    return mChildEntities;
}

//-------------------------------------------------------------------------------------------------

void Entity::notifyANewChildAdded(Entity * entity)
{
    MFA_ASSERT(entity != nullptr);
    MFA_ASSERT(findChild(entity) < 0);
    mChildEntities.emplace_back(entity);
}

//-------------------------------------------------------------------------------------------------

void Entity::notifyAChildRemoved(Entity * entity)
{
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
        component->mInitEventId = mInitSignal.Register([component]()->void{component->Init();});
    }
    // Update event
    if ((component->RequiredEvents() & Component::EventTypes::UpdateEvent) > 0)
    {
        component->mUpdateEventId = mUpdateSignal.Register([component](float const deltaTimeInSec)->void{
            component->Update(deltaTimeInSec);
        });
    }
    // Shutdown event
    if ((component->RequiredEvents() & Component::EventTypes::ShutdownEvent) > 0)
    {
        component->mShutdownEventId = mShutdownSignal.Register([component]()->void{
            component->Shutdown(); 
        });
    }
}

//-------------------------------------------------------------------------------------------------

}
