#pragma once

#include "material/Material.hpp"

class Metal : public Material {
public:

    explicit Metal(glm::vec3 const & color_, float roughness_);

    bool Scatter(
        Ray const & ray,
        HitRecord const & hitRecord,
        glm::vec3 & outAttenuation,
        Ray & outScatteredRay
    ) const override;
    
    float const roughness = 0.0f;

};
