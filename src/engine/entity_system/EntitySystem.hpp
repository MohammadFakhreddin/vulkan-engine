#pragma once

#include "engine/render_system/RenderTypesFWD.hpp"

namespace MFA
{
    class Entity;
}

namespace MFA::EntitySystem {

    void Init();

    void OnNewFrame(float deltaTimeInSec);

    void OnUI();
    
    void Shutdown();

    //using UpdateFunction = std::function<void(float, RT::CommandRecordState const & commandRecord)>;
    //int SubscribeForUpdateEvent(UpdateFunction const & listener);
    //bool UnSubscribeFromUpdateEvent(int listenerId);

    struct CreateEntityParams
    {
        bool serializable = true;   
    };
    Entity * CreateEntity(
        char const * name,
        Entity * parent = nullptr,
        CreateEntityParams const & params = {}
    );

    void InitEntity(Entity * entity, bool triggerSignals = true);

    void UpdateEntity(Entity * entity);

    void DestroyEntity(Entity * entity);

}
