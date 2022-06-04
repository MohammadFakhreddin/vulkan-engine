#include "JsonUtils.hpp"

#include "engine/BedrockAssert.hpp"

#include "libs/nlohmann/json.hpp"

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
        auto const findResult = jsonObject.find(key);
        if (findResult == jsonObject.end())
        {
            MFA_ASSERT(false);
            return;
        }

        auto & rawVec3 = findResult.value();
        outVec3.x = rawVec3.value("x", -1.0f);
        outVec3.y = rawVec3.value("y", -1.0f);
        outVec3.z = rawVec3.value("z", -1.0f);
    }

    //-------------------------------------------------------------------------------------------------

}
