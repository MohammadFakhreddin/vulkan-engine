#include "Sphere.hpp"

#include "glm/glm.hpp"

Sphere::Sphere(glm::vec3 center_, float radius_, glm::vec3 color_)
    : center(std::move(center_))
    , radius(radius_)
    , sqrRadius(radius_ * radius_)
    , color(std::move(color_))
{}

[[nodiscard]]
bool Sphere::HasIntersect(
    Ray const & ray,
    glm::vec3 & outPosition,
    glm::vec3 & outNormal,
    glm::vec3 & outColor
)
{
    auto dir = ray.origin - center;
    float a = glm::dot(ray.direction, ray.direction);
    float b = 2 * glm::dot(ray.direction, dir);
    float c = glm::dot(dir, dir) - sqrRadius;

    auto discriminant = b * b - 4 * a * c;
    if (discriminant < 0) {
        return false;
    }
    
    auto t = (-b - std::sqrt(discriminant)) / (2.0f * a);
    
    outPosition = ray.at(t);
    outNormal = glm::normalize(outPosition - center);
    outColor = color;
    
    return true;
}
