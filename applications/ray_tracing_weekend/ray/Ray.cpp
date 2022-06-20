#include "Ray.hpp"

#include "glm/glm.hpp"

Ray::Ray(glm::vec3 const & origin_, glm::vec3 const & direction_)
    : origin(origin_)
    , direction(glm::normalize(direction_))
{}

glm::vec3 Ray::at(float t) const {
    return origin + direction * t;
}
