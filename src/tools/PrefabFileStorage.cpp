#include "PrefabFileStorage.hpp"

#include "Prefab.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/entity_system/Entity.hpp"

#include "libs/nlohmann/json.hpp"

#include <fstream>

namespace MFA
{
    //-------------------------------------------------------------------------------------------------

    void PrefabFileStorage::Serialize(SerializeParams const & params)
    {
        auto * prefab = params.prefab;
        MFA_ASSERT(prefab != nullptr);
        auto * entity = prefab->GetEntity();
        MFA_ASSERT(entity != nullptr);

        nlohmann::json json {};
        entity->Serialize(json["entity"]);

        std::ofstream file(params.saveAddress);
        file << json;
        file.close();
    }

    //-------------------------------------------------------------------------------------------------

    void PrefabFileStorage::Deserialize(DeserializeParams const & params)
    {
        std::ifstream ifs(params.fileAddress);
        nlohmann::json jf = nlohmann::json::parse(ifs);

        auto * entity = params.prefab->GetEntity();
        MFA_ASSERT(entity != nullptr);
        entity->Deserialize(jf["entity"], params.initializeEntity);
    }

    //-------------------------------------------------------------------------------------------------
}
