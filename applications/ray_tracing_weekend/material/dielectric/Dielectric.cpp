#include "Dielectric.hpp"

#include "geometry/HitRecord.hpp"

#include <glm/gtx/norm.hpp>

#include <algorithm>

//-------------------------------------------------------------------------------------------------

Dielectric::Dielectric(glm::vec3 const & color_, float refractionIndex_) 
    : Material(color_)
    , refractionIndex(refractionIndex_)
{}

//-------------------------------------------------------------------------------------------------
// Section 10.2 Snell's Law
static glm::vec3 Refract(
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

bool Dielectric::Scatter(
    Ray const & ray,
    HitRecord const & hitRecord,
    glm::vec3 & outAttenuation,
    Ray & outScatteredRay
) const {
    
    outAttenuation = color;
    float refractionRatio = hitRecord.hitFrontFace 
        ? (1.0f / refractionIndex) 
        : (refractionIndex / 1.0f);
    
    auto refractDir = Refract(ray.GetDirection(), hitRecord.normal, refractionRatio);
    outScatteredRay = Ray{hitRecord.position, refractDir};
    return true;
}

//-------------------------------------------------------------------------------------------------
