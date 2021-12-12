#include "JsonUtils.hpp"

#include "libs/nlohmann/json.hpp"

#include <glm/vec3.hpp>

namespace MFA::JsonUtils 
{
    //-------------------------------------------------------------------------------------------------

    void SerializeVec3(nlohmann::json & jsonObject, char const * key, glm::vec3 const & vec3)
    {
        nlohmann::json rawVec3 {};
        rawVec3["x"] = vec3.x;
        rawVec3["y"] = vec3.y;
        rawVec3["z"] = vec3.z;

        jsonObject[key] = rawVec3;
    }

    //-------------------------------------------------------------------------------------------------

    void DeserializeVec3(nlohmann::json const & jsonObject, char const * key, glm::vec3 & outVec3)
    {
        auto & rawVec3 = jsonObject[key];
        outVec3.x = rawVec3["x"];
        outVec3.y = rawVec3["y"];
        outVec3.z = rawVec3["z"];
    }

    //-------------------------------------------------------------------------------------------------

}
