#pragma once

namespace MFA
{
    class Entity;

    class Prefab
    {
        explicit Prefab(char const * name, Entity * preBuiltEntity = nullptr);
        ~Prefab();

        struct CloneEntityOptions
        {
            char const * name = nullptr;
        };
        Entity * Clone(Entity * parent, CloneEntityOptions const & options);

    private:

        bool mShouldDestroyEntity = false;
        Entity * mEntity = nullptr;

    };
};
