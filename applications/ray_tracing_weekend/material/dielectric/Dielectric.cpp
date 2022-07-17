#include "Dielectric.hpp"

#include "geometry/HitRecord.hpp"
#include "engine/BedrockMath.hpp"

#include "glm/glm.hpp"

#include <algorithm>

using namespace MFA;

//-------------------------------------------------------------------------------------------------

Dielectric::Dielectric(glm::vec3 const & color_, float refractionIndex_) 
    : Material(color_)
    , refractionIndex(refractionIndex_)
{}

//-------------------------------------------------------------------------------------------------

static float Reflectance(float cosine, float reflectionIndex) {
    // Use Schlick's approximation for reflectance.
    auto r0 = (1 - reflectionIndex) / (1 + reflectionIndex);
    r0 = r0 * r0;
    return r0 + (1 - r0) * pow((1 - cosine), 5);
}

//-------------------------------------------------------------------------------------------------

bool Dielectric::Scatter(
    Ray const & ray,
    HitRecord const & hitRecord,
    glm::vec3 & outAttenuation,
    Ray & outScatteredRay
) const {

    auto rayDir = ray.GetDirection();

    auto cosTheta = std::min(glm::dot(-rayDir, hitRecord.normal), 1.0f);
    auto sinTheta = std::sqrt(1 - cosTheta * cosTheta);
    
    outAttenuation = color;
    
    float refractionRatio = hitRecord.hitFrontFace 
        ? (1.0f / refractionIndex) 
        : (refractionIndex / 1.0f);
    
    auto refractDir = std::abs(sinTheta) > 1 || Reflectance(cosTheta, refractionRatio) > Math::Random(0.0f, 1.0f)
        ? Reflect(rayDir, hitRecord.normal) // Roughness is considered zero here for simplicity
        : Refract(rayDir, hitRecord.normal, refractionRatio);
    
    outScatteredRay = Ray{hitRecord.position, refractDir};

    return true;
}

//-------------------------------------------------------------------------------------------------
