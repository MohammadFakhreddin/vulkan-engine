#pragma once

namespace MFA
{
    class Entity;
}

namespace MFA::EntitySystem {

    void Init();

    void OnNewFrame(float deltaTimeInSec);

    void Shutdown();

    Entity * CreateEntity(char const * name, Entity * parent = nullptr);

    void InitEntity(Entity * entity);

    bool DestroyEntity(Entity * entity, bool shouldNotifyParent = true);

}
