#pragma once

#include "geometry/Geometry.hpp"
#include "ray/Ray.hpp"

#include "glm/vec3.hpp"

class Sphere : public Geometry {
public:

    explicit Sphere(
        glm::vec3 center_, 
        float radius_, 
        std::shared_ptr<Material> material_
    );

    [[nodiscard]]
    bool HasIntersect(
        Ray const & ray,
        float tMin,
        float tMax,
        HitRecord & hitRecord
    ) override;

    glm::vec3 const center;
    
    float const radius;
    float const sqrRadius;
    
};
