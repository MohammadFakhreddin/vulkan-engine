#pragma once

#include "ray/Ray.hpp"

#include "glm/vec3.hpp"

class Sphere {
public:

    explicit Sphere(glm::vec3 center_, float radius_, glm::vec3 color_);

    [[nodiscard]]
    bool HasIntersect(
        Ray const & ray,
        glm::vec3 & outPosition,
        glm::vec3 & outNormal,
        glm::vec3 & outColor
    );

    glm::vec3 const center;
    
    float const radius;
    float const sqrRadius;
    
    glm::vec3 const color;

};
