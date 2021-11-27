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

    void OnResize();

    void Shutdown();

    Entity * CreateEntity(char const * name, Entity * parent = nullptr);

    void InitEntity(Entity * entity);

    void UpdateEntity(Entity * entity);

    bool DestroyEntity(Entity * entity, bool shouldNotifyParent = true);

}
