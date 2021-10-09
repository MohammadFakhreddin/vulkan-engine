#include "EntitySystem.hpp"

#include "Entity.hpp"

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
    std::vector<EntityRef> EntitiesRefsList {};
    Signal<float> UpdateSignal {};
};
State * state = nullptr;

//-------------------------------------------------------------------------------------------------

void Init()
{
    state = new State();   
}

//-------------------------------------------------------------------------------------------------

void OnNewFrame(float deltaTimeInSec)
{
    state->UpdateSignal.Emit(deltaTimeInSec);
}

//-------------------------------------------------------------------------------------------------

void Shutdown()
{
    for (auto & entityRef : state->EntitiesRefsList)
    {
        entityRef.Ptr->Shutdown();
        entityRef.Ptr.reset();
    }
    delete state;
}

//-------------------------------------------------------------------------------------------------

int SubscribeForUpdateEvent(std::function<void(float)> const & listener)
{
    return state->UpdateSignal.Register(listener);
}

//-------------------------------------------------------------------------------------------------

bool UnSubscribeFromUpdateEvent(int const listenerId)
{
    return state->UpdateSignal.UnRegister(listenerId);
}

//-------------------------------------------------------------------------------------------------

Entity * CreateEntity(char const * name, Entity * parent)
{
    state->EntitiesRefsList.emplace_back(EntityRef {
        .Ptr = std::make_unique<Entity>(name, parent)
    });
    return state->EntitiesRefsList.back().Ptr.get();
}

//-------------------------------------------------------------------------------------------------

void InitEntity(Entity * entity)
{
    MFA_ASSERT(entity != nullptr);
    entity->Init();
    if (entity->NeedUpdateEvent()) {
        entity->mUpdateListenerId = state->UpdateSignal.Register([entity](float const deltaTime) -> void {
            entity->Update(deltaTime);
        });
    }
}

//-------------------------------------------------------------------------------------------------

bool DestroyEntity(Entity * entity)
{
    entity->Shutdown();
    if (entity->NeedUpdateEvent())
    {
        state->UpdateSignal.UnRegister(entity->mUpdateListenerId);
    }

    for (int i = static_cast<int>(state->EntitiesRefsList.size()) - 1; i >= 0; --i)
    {
        if (state->EntitiesRefsList[i].Ptr.get() == entity)
        {
            state->EntitiesRefsList[i] = std::move(state->EntitiesRefsList.back());
            state->EntitiesRefsList.pop_back();
            return true;
        }
    }

    return false;
}

//-------------------------------------------------------------------------------------------------

};
