#include "Material.hpp"

#include "engine/BedrockMath.hpp"

#include "glm/glm.hpp"

#include <glm/gtx/norm.hpp>

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

glm::vec3 Material::Reflect(
    glm::vec3 const & vector, 
    glm::vec3 const & normal
) {
    return vector - 2 * glm::dot(vector, normal) * normal;
}

//-------------------------------------------------------------------------------------------------

// Section 10.2 Snell's Law
glm::vec3 Material::Refract(
    glm::vec3 const & rayDir,
    glm::vec3 const & surfNormal,
    float etaiOverEtat
) {
    auto cosTheta = std::min(glm::dot(-rayDir, surfNormal), 1.0f);
    auto rayOutPrep = etaiOverEtat * (rayDir + cosTheta * surfNormal);
    auto rayOutPara = - surfNormal * std::sqrt(std::abs(1.0f - glm::length2(rayOutPrep)));
    return rayOutPrep + rayOutPara;
}

//-------------------------------------------------------------------------------------------------
