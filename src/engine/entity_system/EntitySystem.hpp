#pragma once

#include <string>

namespace MFA
{
    class Entity;
}

namespace MFA::EntitySystem {

    void Init();

    void OnNewFrame(float deltaTimeInSec);

    void OnUI();
    
    void Shutdown();

    struct CreateEntityParams
    {
        bool serializable = true;   
    };
    Entity * CreateEntity(
        std::string const & name,
        Entity * parent = nullptr,
        CreateEntityParams const & params = {}
    );

    void InitEntity(Entity * entity, bool triggerSignals = true);

    void UpdateEntity(Entity * entity);

    void DestroyEntity(Entity * entity);

}
