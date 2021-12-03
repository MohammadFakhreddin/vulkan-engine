#pragma once

#include "engine/render_system/RenderTypesFWD.hpp"

namespace MFA
{
    class Entity;
}

namespace MFA::EntitySystem {

    void Init();

    void OnNewFrame(float deltaTimeInSec, RT::CommandRecordState const & recordState);

    void OnUI();
    
    void Shutdown();

    //using UpdateFunction = std::function<void(float, RT::CommandRecordState const & commandRecord)>;
    //int SubscribeForUpdateEvent(UpdateFunction const & listener);
    //bool UnSubscribeFromUpdateEvent(int listenerId);

    Entity * CreateEntity(char const * name, Entity * parent = nullptr);

    void InitEntity(Entity * entity);

    void UpdateEntity(Entity * entity);

    bool DestroyEntity(Entity * entity, bool shouldNotifyParent = true);

}
