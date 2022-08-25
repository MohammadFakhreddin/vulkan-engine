#include "EntitySystem.hpp"

#include "Entity.hpp"
#include "engine/ui_system/UI_System.hpp"
#include "engine/render_system/RenderTypes.hpp"
#include "engine/scene_manager/Scene.hpp"
#include "engine/scene_manager/SceneManager.hpp"
#include "EntitySystemTypes.hpp"

#include <vector>
#include <memory>

// TODO This class will need optimization in future

namespace MFA::EntitySystem
{
    //constexpr int MAX_ENTITIES = 1024 * 5;

    struct EntityRef
    {
        std::unique_ptr<Entity> ptr;
        //EntityHandle Handle {ENTITY_HANDLE_INVALID};      // We need entityHandle to distinguish entities and be able to remove entities by that
        //bool IsAlive = false;
        //uint32_t NextFreeRef = 0.0f;
    };

    using Task = std::function<void()>;

    struct State
    {
        std::vector<EntityRef> entitiesRefsList{};  // TODO Shouldn't we use map or something faster ?
        Signal<float> updateSignal{};
    };
    State * state = nullptr;

    //-------------------------------------------------------------------------------------------------

    void Init()
    {
        state = new State();
    }

    //-------------------------------------------------------------------------------------------------

    void Update(float const deltaTimeInSec)
    {
        state->updateSignal.EmitMultiThread(deltaTimeInSec);
    }

    //-------------------------------------------------------------------------------------------------

    void OnUI()
    {
        UI::BeginWindow("Entity System");
        UI::Text("Entities:");

        auto const * activeScene = SceneManager::GetActiveScene();
        if (activeScene != nullptr)
        {
            auto * rootEntity = activeScene->GetRootEntity();
            rootEntity->OnUI();
        }

        UI::EndWindow();
    }

    //-------------------------------------------------------------------------------------------------

    void Shutdown()
    {
        for (auto const & entityRef : state->entitiesRefsList)
        {
            entityRef.ptr->Shutdown();
        }
        delete state;
    }

    //-------------------------------------------------------------------------------------------------

    Entity * CreateEntity(
        std::string const & name,
        Entity * parent,
        CreateEntityParams const & params
    )
    {
        static EntityId nextEntityId = 0;
        // Checking if we have not reached maximum possible entity limit
        MFA_ASSERT(nextEntityId < std::numeric_limits<EntityId>::max());

        char nameBuffer[100] {};
        auto const nameLength = sprintf(nameBuffer, "%s %d", name.c_str(), static_cast<int>(nextEntityId));

        state->entitiesRefsList.emplace_back(EntityRef{
            .ptr = std::make_unique<Entity>(
                nextEntityId++,
                std::string(nameBuffer, nameLength),
                parent,
                params.serializable
            )
        });
        return state->entitiesRefsList.back().ptr.get();
    }

    //-------------------------------------------------------------------------------------------------

    void InitEntity(Entity * entity, bool const triggerSignals)
    {
        MFA_ASSERT(entity != nullptr);
        entity->Init(triggerSignals);
        entity->LateInit(triggerSignals);
        if (triggerSignals)
        {
            UpdateEntity(entity);
        }
    }

    //-------------------------------------------------------------------------------------------------

    void UpdateEntity(Entity * entity)
    {
        MFA_ASSERT(entity != nullptr);
        MFA_ASSERT(entity->mIsInitialized == true);
        if (entity->NeedUpdateEvent())
        {
            if (entity->mUpdateListenerId == SignalIdInvalid)
            {
                entity->mUpdateListenerId = state->updateSignal.Register([entity](float const deltaTime) -> void
                {
                    entity->Update(deltaTime);
                });
            }
        }
        else
        {
            state->updateSignal.UnRegister(entity->mUpdateListenerId);
            entity->mUpdateListenerId = SignalIdInvalid;
        }
    }

    //-------------------------------------------------------------------------------------------------

    static void destroyEntity(EntityId const entityId, bool const shouldNotifyParent)
    {
        for (int i = static_cast<int>(state->entitiesRefsList.size()) - 1; i >= 0; --i)
        {
            if (state->entitiesRefsList[i].ptr->getId() == entityId)
            {
                std::unique_ptr<Entity> const deletedEntity = std::move(state->entitiesRefsList[i].ptr);
                MFA_ASSERT(deletedEntity != nullptr);

                // We should remove item before removing children because index might change
                state->entitiesRefsList[i] = std::move(state->entitiesRefsList.back());
                state->entitiesRefsList.pop_back();

                deletedEntity->Shutdown(shouldNotifyParent);
                
                if (deletedEntity->NeedUpdateEvent())
                {
                    state->updateSignal.UnRegister(deletedEntity->mUpdateListenerId);
                }

                for (auto * childEntity : deletedEntity->GetChildEntities())
                {
                    MFA_ASSERT(childEntity != nullptr);
                    destroyEntity(childEntity->getId(), false);
                }

                return;
            }
        }

        MFA_LOG_WARN("Entity with id %d is not found", static_cast<int>(entityId));
    }

    //-------------------------------------------------------------------------------------------------

    void DestroyEntity(Entity * entity)
    {
        MFA_ASSERT(entity != nullptr);

        auto const entityId = entity->getId();
        destroyEntity(entityId, true);
    }

    //-------------------------------------------------------------------------------------------------

};
