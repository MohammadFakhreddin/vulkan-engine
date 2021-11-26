#include "EntitySystem.hpp"

#include "Entity.hpp"
#include "engine/ui_system/UISystem.hpp"
#include "engine/render_system/RenderTypes.hpp"

#include <vector>
#include <memory>

#include "engine/scene_manager/Scene.hpp"
#include "engine/scene_manager/SceneManager.hpp"

// TODO This class will need optimization in future

namespace MFA::EntitySystem {

//constexpr int MAX_ENTITIES = 1024 * 5;

struct EntityRef {
    std::unique_ptr<Entity> Ptr {};
    //EntityHandle Handle {ENTITY_HANDLE_INVALID};      // We need entityHandle to distinguish entities and be able to remove entities by that
    //bool IsAlive = false;
    //uint32_t NextFreeRef = 0.0f;
};

struct State {
    std::vector<EntityRef> entitiesRefsList {};
    Signal<float, RT::CommandRecordState const &> updateSignal {};
};
State * state = nullptr;

//-------------------------------------------------------------------------------------------------

void Init()
{
    state = new State();
}

//-------------------------------------------------------------------------------------------------

void OnNewFrame(float deltaTimeInSec, RT::CommandRecordState const & recordState)
{
    state->updateSignal.Emit(deltaTimeInSec, recordState);
}

//-------------------------------------------------------------------------------------------------

void OnUI()
{
    UI::BeginWindow("Entity System");
    UI::Text("Entities:");
    auto const & activeScene = SceneManager::GetActiveScene();
    if (auto const activeScenePtr = activeScene.lock())
    {
        auto * rootEntity = activeScenePtr->GetRootEntity();
        rootEntity->OnUI();
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
    delete state;
}

//-------------------------------------------------------------------------------------------------

int SubscribeForUpdateEvent(std::function<void(float, RT::CommandRecordState const & commandRecord)> const & listener)
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
        entity->mUpdateListenerId = state->updateSignal.Register([entity](
            float const deltaTime,
            RT::CommandRecordState const & recordState
        ) -> void {
            entity->Update(deltaTime, recordState);
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
