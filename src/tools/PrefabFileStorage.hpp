#pragma once

class Entity;

namespace MFA::PrefabFileStorage
{
    // TODO I'm still not sure about this file. I might remove it
    struct SerializeParams
    {
        char const * saveAddress;
        char const * prefabName;
        Entity * rootEntity;
    };
    void Serialize(SerializeParams const & params);

    struct DeserializeParams
    {
        char const * fileAddress;           // I think we should use fileAddress for essence name otherwise their uniqueness will be a problem
    };
    void Deserialize(DeserializeParams const & params);

}
