#pragma once

#include "libs/nlohmann/json_fwd.hpp"

#include <glm/fwd.hpp>

namespace MFA::JsonUtils 
{

    void SerializeVec3(nlohmann::json & jsonObject, char const * key, glm::vec3 const & vec3);

    void DeserializeVec3(nlohmann::json const & jsonObject, char const * key, glm::vec3 & outVec3);

}
