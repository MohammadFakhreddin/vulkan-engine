#include "EntitySystem.hpp"

#include "Entity.hpp"

#include <vector>
#include <memory>

#include "EntityHandle.hpp"

// TODO This class will need optimization in future

namespace MFA::EntitySystem {

//constexpr int MAX_ENTITIES = 1024 * 5;

struct EntityRef {
    std::unique_ptr<Entity> Ptr = nullptr;
    //EntityHandle Handle {ENTITY_HANDLE_INVALID};
    //bool IsAlive = false;
    //uint32_t NextFreeRef = 0.0f;
};

struct State {
    std::vector<EntityRef> entitiesRef {};
};

//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------

};