#include "Metal.hpp"

#include "geometry/HitRecord.hpp"
#include "engine/BedrockMatrix.hpp"

#include "glm/glm.hpp"

using namespace MFA;

//-------------------------------------------------------------------------------------------------

Metal::Metal(glm::vec3 const & color_, float roughness_)
    : Material(color_)
    , roughness(roughness_)
{}

//-------------------------------------------------------------------------------------------------

static glm::vec3 reflect(
    glm::vec3 const & vector, 
    glm::vec3 const & normal
) {
    return vector - 2 * glm::dot(vector, normal) * normal;
}

//-------------------------------------------------------------------------------------------------

bool Metal::Scatter(
    Ray const & ray,
    HitRecord const & hitRecord,
    glm::vec3 & outAttenuation,
    Ray & outScatteredRay
) const 
{
    auto direction = reflect(ray.GetDirection(), hitRecord.normal);
    if (roughness > Math::Epsilon<float>()) {
        direction += roughness * RandomUnitVector();
    }
    outScatteredRay = Ray {hitRecord.position, direction};
    outAttenuation = color;
    return glm::dot(direction, hitRecord.normal) > 0;
}

//-------------------------------------------------------------------------------------------------
