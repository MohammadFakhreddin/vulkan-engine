#pragma once

#include "material/Material.hpp"

#include <glm/vec3.hpp>

class Dielectric : public Material {
public:

    explicit Dielectric(glm::vec3 const & color_, float refractionIndex_);
    
    bool Scatter(
        Ray const & ray,
        HitRecord const & hitRecord,
        glm::vec3 & outAttenuation,
        Ray & outScatteredRay
    ) const override;

    float const refractionIndex = 0.0f;
};