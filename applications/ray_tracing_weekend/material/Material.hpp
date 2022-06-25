#pragma once

#include "ray/Ray.hpp"

#include "glm/vec3.hpp"

struct HitRecord;

class Material {
public:

    explicit Material(glm::vec3 color_);

    virtual bool Scatter(
        Ray const & ray,
        HitRecord const & hitRecord,
        glm::vec3 & outAttenuation,
        Ray & outScatteredRay
    ) const = 0;

    static glm::vec3 RandomVec3(float min, float max);

    static glm::vec3 RandomUnitVector();

    glm::vec3 const color {};

};
