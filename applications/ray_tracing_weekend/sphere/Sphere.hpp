#pragma once

#include "geometry/Geometry.hpp"
#include "ray/Ray.hpp"

#include "glm/vec3.hpp"

class Sphere : public Geometry {
public:

    explicit Sphere(glm::vec3 center_, float radius_, glm::vec3 color_);

    [[nodiscard]]
    bool HasIntersect(
        Ray const & ray,
        float tMin,
        float tMax,
        glm::vec3 & outPosition,
        glm::vec3 & outNormal,
        glm::vec3 & outColor
    ) override;

    glm::vec3 const center;
    
    float const radius;
    float const sqrRadius;
    
    glm::vec3 const color;

};
