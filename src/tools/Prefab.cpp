#include "Prefab.hpp"

#include "engine/entity_system/Entity.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    Prefab::Prefab(Entity * preBuiltEntity)
        : mOwnedEntity(nullptr)
        , mEntity(nullptr)
    {
        MFA_ASSERT(preBuiltEntity != nullptr);
        mEntity = preBuiltEntity;
    }

    //-------------------------------------------------------------------------------------------------

    Prefab::~Prefab() = default;

    //-------------------------------------------------------------------------------------------------

    Entity * Prefab::Clone(Entity * parent, CloneEntityOptions const & options)
    {
        MFA_ASSERT(mEntity != nullptr);
        std::string const name = options.name.empty()
            ? std::format("%s Clone(%d)", mEntity->GetName().c_str(), cloneCount++)
            : mEntity->GetName();

        auto * entity = mEntity->Clone(name.c_str(), parent);
        EntitySystem::InitEntity(entity);
        return entity;
    }

    //-------------------------------------------------------------------------------------------------

}
