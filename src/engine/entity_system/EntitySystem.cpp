#include "EntitySystem.hpp"

#include "Entity.hpp"
#include "engine/ui_system/UISystem.hpp"

#include <vector>
#include <memory>

// TODO This class will need optimization in future

namespace MFA::EntitySystem {

//constexpr int MAX_ENTITIES = 1024 * 5;

struct EntityRef {
    std::unique_ptr<Entity> Ptr {};
    //EntityHandle Handle {ENTITY_HANDLE_INVALID};
    //bool IsAlive = false;
    //uint32_t NextFreeRef = 0.0f;
};

struct State {
    std::vector<EntityRef> entitiesRefsList {};
    Signal<float> updateSignal {};
    int uiListenerId = 0;
    // TODO List of lights!
};
State * state = nullptr;

//-------------------------------------------------------------------------------------------------

void Init()
{
    state = new State();
    state->uiListenerId = UISystem::Register([]()->void{
        OnUI();
    });
}

//-------------------------------------------------------------------------------------------------

void OnNewFrame(float deltaTimeInSec)
{
    state->updateSignal.Emit(deltaTimeInSec);
}

//-------------------------------------------------------------------------------------------------

void OnUI()
{
    UI::BeginWindow("Entity System");
    UI::Text("Entities:");
    for (auto const & entityRef : state->entitiesRefsList)
    {
        if (entityRef.Ptr->HasParent() == false)
        {
            entityRef.Ptr->OnUI();
        }
    }
    UI::EndWindow();
}

//-------------------------------------------------------------------------------------------------

void Shutdown()
{
    for (auto const & entityRef : state->entitiesRefsList)
    {
        entityRef.Ptr->Shutdown();
    }
    UISystem::UnRegister(state->uiListenerId);
    delete state;
}

//-------------------------------------------------------------------------------------------------

int SubscribeForUpdateEvent(std::function<void(float)> const & listener)
{
    return state->updateSignal.Register(listener);
}

//-------------------------------------------------------------------------------------------------

bool UnSubscribeFromUpdateEvent(int const listenerId)
{
    return state->updateSignal.UnRegister(listenerId);
}

//-------------------------------------------------------------------------------------------------

Entity * CreateEntity(char const * name, Entity * parent)
{
    state->entitiesRefsList.emplace_back(EntityRef {
        .Ptr = std::make_unique<Entity>(name, parent)
    });
    return state->entitiesRefsList.back().Ptr.get();
}

//-------------------------------------------------------------------------------------------------

void InitEntity(Entity * entity)
{
    MFA_ASSERT(entity != nullptr);
    entity->Init();
    if (entity->NeedUpdateEvent()) {
        entity->mUpdateListenerId = state->updateSignal.Register([entity](float const deltaTime) -> void {
            entity->Update(deltaTime);
        });
    }
}

//-------------------------------------------------------------------------------------------------

bool DestroyEntity(Entity * entity, bool const shouldNotifyParent)
{
    MFA_ASSERT(entity != nullptr);
    entity->Shutdown(shouldNotifyParent);
    if (entity->NeedUpdateEvent())
    {
        state->updateSignal.UnRegister(entity->mUpdateListenerId);
    }
    for (auto * childEntity : entity->GetChildEntities())
    {
        MFA_ASSERT(childEntity != nullptr);
        DestroyEntity(childEntity, false);
    }

    for (int i = static_cast<int>(state->entitiesRefsList.size()) - 1; i >= 0; --i)
    {
        if (state->entitiesRefsList[i].Ptr.get() == entity)
        {
            state->entitiesRefsList[i] = std::move(state->entitiesRefsList.back());
            state->entitiesRefsList.pop_back();
            return true;
        }
    }

    return false;
}

//-------------------------------------------------------------------------------------------------

};
