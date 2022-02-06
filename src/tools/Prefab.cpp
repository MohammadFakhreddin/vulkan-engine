#include "Prefab.hpp"

#include "engine/entity_system/Entity.hpp"
#include "engine/entity_system/EntitySystem.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    Prefab::Prefab(Entity * preBuiltEntity)
        : mEntity(preBuiltEntity)
    {
        MFA_ASSERT(preBuiltEntity != nullptr);
    }
    
    //-------------------------------------------------------------------------------------------------

    Prefab::~Prefab()
    {
        EntitySystem::DestroyEntity(mEntity);
    }
    
    //-------------------------------------------------------------------------------------------------

    Entity * Prefab::Clone(Entity * parent, CloneEntityOptions const & options)
    {
        MFA_ASSERT(mEntity != nullptr);
        std::string const name = options.name.empty()
            ? std::format("%s Clone(%d)", mEntity->GetName().c_str(), cloneCount++)
            : options.name;

        auto * entity = mEntity->Clone(name.c_str(), parent);
        EntitySystem::InitEntity(entity, true);
        return entity;
    }

    //-------------------------------------------------------------------------------------------------

    Entity * Prefab::GetEntity() const
    {
        return mEntity;
    }

    //-------------------------------------------------------------------------------------------------

    void Prefab::AssignPreBuiltEntity(Entity * preBuiltEntity)
    {
        MFA_ASSERT(preBuiltEntity != nullptr);
        mEntity = preBuiltEntity;
    }

    //-------------------------------------------------------------------------------------------------

}
