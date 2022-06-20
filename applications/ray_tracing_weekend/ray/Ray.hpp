#pragma once

#include "glm/vec3.hpp"

class Ray {
public:

    explicit Ray(glm::vec3 const & origin_, glm::vec3 const & direction_);

    [[nodiscard]]
    glm::vec3 at(float t) const;

    glm::vec3 const origin;
    glm::vec3 const direction;

};
