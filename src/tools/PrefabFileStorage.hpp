#pragma once

#include <string>

namespace MFA
{
    class Prefab;
    class Entity;
}

// TODO We can move these functions into prefab
namespace MFA::PrefabFileStorage
{
    struct SerializeParams
    {
        std::string saveAddress {};
        Prefab * prefab = nullptr;
    };
    void Serialize(SerializeParams const & params);

    struct DeserializeParams
    {
        std::string fileAddress {};           // I think we should use fileAddress for essence name otherwise their uniqueness will be a problem
        Prefab * prefab = nullptr;
        bool initializeEntity = false;
    };
    void Deserialize(DeserializeParams const & params);

}
