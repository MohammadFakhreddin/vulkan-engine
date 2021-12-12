#include "Prefab.hpp"

#include "engine/entity_system/Entity.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    Prefab::Prefab() = default;

    //-------------------------------------------------------------------------------------------------

    Prefab::Prefab(Entity * preBuiltEntity)
        : mOwnedEntity(nullptr)
        , mEntity(preBuiltEntity)
    {
        MFA_ASSERT(preBuiltEntity != nullptr);
    }

    //-------------------------------------------------------------------------------------------------

    Prefab::Prefab(std::unique_ptr<Entity> && ownedEntity)
        : mOwnedEntity(std::move(ownedEntity))
        , mEntity(mOwnedEntity.get())
    {}

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

    Entity * Prefab::GetEntity() const
    {
        return mEntity;
    }

    //-------------------------------------------------------------------------------------------------

    void Prefab::AssignPreBuiltEntity(Entity * preBuiltEntity)
    {
        MFA_ASSERT(preBuiltEntity != nullptr);
        mOwnedEntity.reset();
        mEntity = preBuiltEntity;
    }

    //-------------------------------------------------------------------------------------------------

}
