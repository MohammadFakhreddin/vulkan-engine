#pragma once

#include "libs/nlohmann/json.hpp"

#include <glm/fwd.hpp>

namespace MFA::JsonUtils 
{

    void SerializeVec3(nlohmann::json & jsonObject, char const * key, glm::vec3 const & vec3);

    void DeserializeVec3(nlohmann::json const & jsonObject, char const * key, glm::vec3 & outVec3);

    template<typename ParameterType>
    static ParameterType TryGetValue(
        nlohmann::json const & jsonObject,
        char const * keyName,
        ParameterType defaultValue
    )
    {
        ParameterType result = defaultValue;
        const auto findResult = jsonObject.find(keyName);
        if (findResult != jsonObject.end())
        {
            result = findResult.value().get<ParameterType>(defaultValue);
        }
        return result;
    }

}
