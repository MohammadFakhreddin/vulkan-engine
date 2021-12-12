#pragma once

#include <memory>
#include <string>

namespace MFA
{
    class Entity;

    class Prefab
    {
    public:

        explicit Prefab();
        explicit Prefab(Entity * preBuiltEntity);
        explicit Prefab(std::unique_ptr<Entity> && ownedEntity);
        ~Prefab();

        struct CloneEntityOptions
        {
            std::string name {};
        };
        Entity * Clone(Entity * parent, CloneEntityOptions const & options);

        Prefab(Prefab const &) noexcept = delete;
        Prefab(Prefab &&) noexcept = delete;
        Prefab & operator = (Prefab const &) noexcept = delete;
        Prefab & operator = (Prefab && rhs) noexcept = delete;

        [[nodiscard]]
        Entity * GetEntity() const;

        void AssignPreBuiltEntity(Entity * preBuiltEntity);

    private:

        std::unique_ptr<Entity> mOwnedEntity;
        Entity * mEntity;                       // I can be owned or prebuilt

        int cloneCount = 0;

    };
};
