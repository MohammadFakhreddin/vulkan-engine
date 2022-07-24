#pragma once

#include "material/Material.hpp"

class Diffuse : public Material {
public:

    using Material::Material;

    bool Scatter(
        Ray const & ray,
        HitRecord const & hitRecord,
        glm::vec3 & outAttenuation,
        Ray & outScatteredRay
    ) const override;

};
