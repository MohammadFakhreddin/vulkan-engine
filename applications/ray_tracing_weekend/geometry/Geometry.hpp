#pragma once

#include "ray/Ray.hpp"

#include "glm/vec3.hpp"

class Geometry {
public:

    [[nodiscard]]
    virtual bool HasIntersect(
        Ray const & ray,
        float tMin,
        float tMax,
        glm::vec3 & outPosition,
        glm::vec3 & outNormal,
        glm::vec3 & outColor
    ) = 0;

};
