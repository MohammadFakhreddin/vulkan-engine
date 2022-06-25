#include "Material.hpp"

#include "engine/BedrockMath.hpp"

#include "glm/glm.hpp"

using namespace MFA;

//-------------------------------------------------------------------------------------------------

Material::Material(glm::vec3 color_) : color(color_) {} 

//-------------------------------------------------------------------------------------------------

glm::vec3 Material::RandomVec3(float min, float max) {
    return glm::vec3 {
        Math::Random(min, max),
        Math::Random(min, max),
        Math::Random(min, max)
    };
}

//-------------------------------------------------------------------------------------------------

glm::vec3 Material::RandomUnitVector() {
    return glm::normalize(RandomVec3(-1.0f, 1.0f));
}

//-------------------------------------------------------------------------------------------------
