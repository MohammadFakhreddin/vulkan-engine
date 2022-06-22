#pragma once

#include "glm/glm.hpp"

class Ray {
public:

    explicit Ray(glm::vec3 const & origin_, glm::vec3 const & direction_) 
        : origin(origin_)
        , direction(glm::normalize(direction_))
    {}

    glm::vec3 at(float t) {
        return origin + direction * t;
    }

    glm::vec3 const origin;
    glm::vec3 const direction;

};
