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

    static glm::vec3 Reflect(
        glm::vec3 const & vector, 
        glm::vec3 const & normal
    );

    static glm::vec3 Refract(
        glm::vec3 const & rayDir,
        glm::vec3 const & surfNormal,
        float etaiOverEtat
    );

    glm::vec3 const color {};

};
